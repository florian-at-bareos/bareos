/*
   BAREOS - Backup Archiving REcovery Open Sourced

   Copyright (C) 2012 Planets Communications B.V.
   Copyright (C) 2013-2024 Bareos GmbH & Co. KG

   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.
*/
// Marco van Wieringen, November 2012
/**
 * @file
 * This file handles commands from another Storage daemon.
 *
 * We get here because the Director has initiated a Job with
 * another Storage daemon, then done the same with this
 * Storage daemon. When the Storage daemon receives a proper
 * connection from the other Storage daemon, control is
 * passed here to handle the subsequent Storage daemon commands.
 */

#include "include/bareos.h"
#include "stored/stored.h"
#include "stored/stored_globals.h"
#include "stored/append.h"
#include "stored/authenticate.h"
#include "stored/stored_jcr_impl.h"
#include "stored/sd_stats.h"
#include "stored/sd_stats.h"
#include "lib/bnet.h"
#include "lib/bsock.h"
#include "lib/edit.h"
#include "include/jcr.h"

namespace storagedaemon {

/* Imported variables */

/* Static variables */
static char serrmsg[] = "3900 Invalid command\n";

/* Imported functions */

/* Forward referenced SD commands */
static bool StartReplicationSession(JobControlRecord* jcr);
static bool ReplicateData(JobControlRecord* jcr);
static bool EndReplicationSession(JobControlRecord* jcr);

struct s_sd_cmds {
  const char* cmd;
  bool (*func)(JobControlRecord* jcr);
};

// The following are the recognized commands from the Remote Storage daemon
static struct s_sd_cmds sd_cmds[] = {
    {"start replicate", StartReplicationSession},
    {"replicate data", ReplicateData},
    {"end replicate", EndReplicationSession},
    {NULL, NULL} /* list terminator */
};

// Responses sent to the Remote Storage daemon
static char NO_open[] = "3901 Error replicate session already open\n";
static char NOT_opened[] = "3902 Error replicate session not opened\n";
static char ERROR_replicate[] = "3903 Error replicate data\n";
static char OK_end_replicate[] = "3000 OK end replicate\n";
static char OK_start_replicate[] = "3000 OK start replicate ticket = %d\n";

// Responses sent to the Director
static char Job_start[] = "3010 Job %s start\n";
static char Job_end[]
    = "3099 Job %s end JobStatus=%d JobFiles=%d JobBytes=%s JobErrors=%u\n";

/**
 * After receiving a connection (in socket_server.c) if it is
 * from the Storage daemon, this routine is called.
 */
void* handle_stored_connection(BareosSocket* sd, char* job_name)
{
  JobControlRecord* jcr;

  /* With the following Bmicrosleep on, running the
   * SD under the debugger fails. */
  // Bmicrosleep(0, 50000);             /* wait 50 millisecs */
  if (!(jcr = get_jcr_by_full_name(job_name))) {
    Jmsg1(NULL, M_FATAL, 0, T_("SD connect failed: Job name not found: %s\n"),
          job_name);
    Dmsg1(3, "**** Job \"{}\" not found.\n", job_name);
    sd->close();
    delete sd;
    return NULL;
  }

  Dmsg1(50, "Found Job {}\n", job_name);

  if (jcr->authenticated) {
    Jmsg2(jcr, M_FATAL, 0,
          T_("Hey!!!! JobId %u Job %s already authenticated.\n"),
          (uint32_t)jcr->JobId, jcr->Job);
    Dmsg2(50, "Hey!!!! JobId {} Job {} already authenticated.\n",
          (uint32_t)jcr->JobId, jcr->Job);
    sd->close();
    delete sd;
    FreeJcr(jcr);
    return NULL;
  }

  jcr->store_bsock = sd;
  jcr->store_bsock->SetJcr(jcr);

  // Authenticate the Storage daemon
  if (jcr->authenticated || !AuthenticateStoragedaemon(jcr)) {
    Dmsg1(50, "Authentication failed Job {}\n", jcr->Job);
    Jmsg(jcr, M_FATAL, 0, T_("Unable to authenticate Storage daemon\n"));
  } else {
    *jcr->sd_impl->client_available.lock() = true;
    Dmsg2(50, "OK Authentication jid={} Job {}\n", (uint32_t)jcr->JobId,
          jcr->Job);
  }

  if (!jcr->authenticated) {
    jcr->setJobStatusWithPriorityCheck(JS_ErrorTerminated);
  }

  jcr->sd_impl->job_start_wait.notify_one(); /* wake waiting job */
  FreeJcr(jcr);

  return NULL;
}

// Now talk to the SD and do what he says
static void DoSdCommands(JobControlRecord* jcr)
{
  int i, status;
  bool found, quit;
  BareosSocket* sd = jcr->store_bsock;

  sd->SetJcr(jcr);
  quit = false;
  while (!quit) {
    // Read command coming from the Storage daemon
    status = sd->recv();
    if (IsBnetStop(sd)) { /* hardeof or error */
      break;              /* connection terminated */
    }
    if (status <= 0) { continue; /* ignore signals and zero length msgs */ }

    Dmsg1(110, "<stored: {}", sd->msg);
    found = false;
    for (i = 0; sd_cmds[i].cmd; i++) {
      if (bstrncmp(sd_cmds[i].cmd, sd->msg, strlen(sd_cmds[i].cmd))) {
        found = true; /* indicate command found */
        jcr->errmsg[0] = 0;
        if (!sd_cmds[i].func(jcr)) { /* do command */
          // Note sd->msg command may be destroyed by comm activity
          if (!jcr->IsJobCanceled()) {
            if (jcr->errmsg[0]) {
              Jmsg1(jcr, M_FATAL, 0,
                    T_("Command error with SD, hanging up. %s\n"), jcr->errmsg);
            } else {
              Jmsg0(jcr, M_FATAL, 0,
                    T_("Command error with SD, hanging up.\n"));
            }
            jcr->setJobStatusWithPriorityCheck(JS_ErrorTerminated);
          }
          quit = true;
        }
        break;
      }
    }

    if (!found) { /* command not found */
      if (!jcr->IsJobCanceled()) {
        Jmsg1(jcr, M_FATAL, 0, T_("SD command not found: %s\n"), sd->msg);
        Dmsg1(110, "<stored: Command not found: {}\n", sd->msg);
      }
      sd->fsend(serrmsg);
      break;
    }
  }
  sd->signal(BNET_TERMINATE); /* signal to SD job is done */
}

/**
 * Run a Storage daemon replicate Job -- Wait for remote Storage daemon
 * to connect and authenticate it we then will get a wakeup sign using
 * the job_start_wait conditional
 *
 * Director sends us this command.
 *
 * Basic task here is:
 * - Read a command from the Storage daemon
 * - Execute it
 */
bool DoListenRun(JobControlRecord* jcr)
{
  char ec1[30];
  BareosSocket* dir = jcr->dir_bsock;

  jcr->sendJobStatus(JS_WaitSD); /* wait for SD to connect */

  Dmsg2(50, "{} waiting for SD to contact SD key={}\n", jcr->Job,
        jcr->sd_auth_key);
  Dmsg2(800, "Wait SD for jid={} {}\n", jcr->JobId, jcr);

  /* Wait for the Storage daemon to contact us to start the Job, when he does,
   * we will be released. */
  {
    auto locked = jcr->sd_impl->client_available.lock();
    locked.wait(jcr->sd_impl->job_start_wait, [jcr](bool started) {
      return started || jcr->IsJobCanceled();
    });
  }
  Dmsg3(50, "Auth={} canceled={}\n", jcr->authenticated, jcr->IsJobCanceled());

  if (!jcr->authenticated || !jcr->store_bsock) {
    Dmsg2(800, "Auth fail or cancel for jid={} {}\n", jcr->JobId, jcr);
    DequeueMessages(jcr); /* send any queued messages */

    goto cleanup;
  }

  Dmsg1(120, "Start run Job={}\n", jcr->Job);

  dir->fsend(Job_start, jcr->Job);
  jcr->start_time = time(NULL);
  jcr->run_time = jcr->start_time;
  jcr->sendJobStatus(JS_Running);

  // See if we need to limit the inbound bandwidth.
  if (me->max_bandwidth_per_job && jcr->store_bsock) {
    jcr->store_bsock->SetBwlimit(me->max_bandwidth_per_job);
    if (me->allow_bw_bursting) { jcr->store_bsock->SetBwlimitBursting(); }
  }

  DoSdCommands(jcr);

  jcr->end_time = time(NULL);

  DequeueMessages(jcr); /* send any queued messages */
  jcr->setJobStatusWithPriorityCheck(JS_Terminated);

cleanup:
  GeneratePluginEvent(jcr, bSdEventJobEnd);

  dir->fsend(Job_end, jcr->Job, jcr->getJobStatus(), jcr->JobFiles,
             edit_uint64(jcr->JobBytes, ec1), jcr->JobErrors);

  dir->signal(BNET_EOD); /* send EOD to Director daemon */

  FreePlugins(jcr); /* release instantiated plugins */

  // After a listen cmd we are done e.g. return false.
  return false;
}

// Start of replication.
static bool StartReplicationSession(JobControlRecord* jcr)
{
  BareosSocket* sd = jcr->store_bsock;

  Dmsg1(120, "Start replication session: {}", sd->msg);
  if (jcr->sd_impl->session_opened) {
    PmStrcpy(jcr->errmsg, T_("Attempt to open already open session.\n"));
    sd->fsend(NO_open);
    return false;
  }

  jcr->sd_impl->session_opened = true;

  // Send "Ticket" to Storage Daemon
  sd->fsend(OK_start_replicate, jcr->VolSessionId);
  Dmsg1(110, ">stored: {}", sd->msg);

  return true;
}

/**
 * Replicate data.
 *    Open Data Channel and receive Data for archiving
 *    Write the Data to the archive device
 */
static bool ReplicateData(JobControlRecord* jcr)
{
  BareosSocket* sd = jcr->store_bsock;

  Dmsg1(120, "Replicate data: {}", sd->msg);
  if (jcr->sd_impl->session_opened) {
    utime_t now;

    // Update the initial Job Statistics.
    now = (utime_t)time(NULL);
    UpdateJobStatistics(jcr, now);

    Dmsg1(110, "<stored: {}", sd->msg);
    if (DoAppendData(jcr, sd, "SD")) {
      return true;
    } else {
      PmStrcpy(jcr->errmsg, T_("Replicate data error.\n"));
      BnetSuppressErrorMessages(sd, 1); /* ignore errors at this point */
      sd->fsend(ERROR_replicate);
    }
  } else {
    PmStrcpy(jcr->errmsg, T_("Attempt to replicate on non-open session.\n"));
    sd->fsend(NOT_opened);
  }

  return false;
}

// End a replication session.
static bool EndReplicationSession(JobControlRecord* jcr)
{
  BareosSocket* sd = jcr->store_bsock;

  Dmsg1(120, "stored<stored: {}", sd->msg);
  if (!jcr->sd_impl->session_opened) {
    PmStrcpy(jcr->errmsg, T_("Attempt to close non-open session.\n"));
    sd->fsend(NOT_opened);
    return false;
  }
  return sd->fsend(OK_end_replicate);
}

} /* namespace storagedaemon */
