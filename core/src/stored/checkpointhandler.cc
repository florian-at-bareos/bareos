/*
   BAREOS® - Backup Archiving REcovery Open Sourced

Copyright (C) 2023-2023 Bareos GmbH & Co. KG

This program is Free Software; you can redistribute it and/or
modify it under the terms of version three of the GNU Affero General Public
License as published by the Free Software Foundation and included
in the file LICENSE.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.
*/

#include "checkpointhandler.h"

#include "stored/stored_jcr_impl.h"
#include "stored/device_control_record.h"

namespace storagedaemon {

CheckpointHandler::CheckpointHandler(time_t interval)
    : checkpoint_interval_(interval)
{
  auto now
      = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  this->next_checkpoint_time_ = now + checkpoint_interval_;
}


void CheckpointHandler::UpdateFileList(JobControlRecord* jcr)
{
  Dmsg0(100, _("... update file list\n"));
  jcr->sd_impl->dcr->DirAskToUpdateFileList();
}

void CheckpointHandler::UpdateJobmediaRecord(JobControlRecord* jcr)
{
  Dmsg0(100, _("... create job media record\n"));
  jcr->sd_impl->dcr->DirCreateJobmediaRecord(false);
}

void CheckpointHandler::UpdateJobrecord(JobControlRecord* jcr)
{
  Dmsg2(100, _("... update job record: %llu bytes %lu files\n"), jcr->JobBytes,
        jcr->JobFiles);
  jcr->sd_impl->dcr->DirAskToUpdateJobRecord();
}

void CheckpointHandler::DoBackupCheckpoint(JobControlRecord* jcr)
{
  Dmsg0(100, _("Checkpoint: Syncing current backup status to catalog\n"));
  UpdateJobrecord(jcr);
  UpdateFileList(jcr);
  UpdateJobmediaRecord(jcr);

  jcr->sd_impl->dcr->VolFirstIndex = jcr->sd_impl->dcr->VolLastIndex;

  SetReadyForCheckpoint(false);

  Dmsg0(100, _("Checkpoint completed\n"));
}

/* On volume changes, the SD already creates a jobmedia table entry for the
   finished volume, so we only need to update the File and Job tables */
void CheckpointHandler::DoVolumeChangeBackupCheckpoint(JobControlRecord* jcr)
{
  Jmsg0(jcr, M_INFO, 0, _("Volume changed, doing checkpoint:\n"));
  Dmsg0(100, _("Checkpoint: Syncing current backup status to catalog\n"));
  UpdateJobrecord(jcr);
  UpdateFileList(jcr);

  SetReadyForCheckpoint(false);

  Dmsg0(100, _("Checkpoint completed\n"));
}

void CheckpointHandler::DoTimedCheckpoint(JobControlRecord* jcr)
{
  const time_t now
      = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  if (now > next_checkpoint_time_) {
    while (next_checkpoint_time_ <= now) {
      next_checkpoint_time_ += checkpoint_interval_;
    }
    Jmsg(jcr, M_INFO, 0,
         _("Doing timed backup checkpoint. Next checkpoint in %d seconds\n"),
         checkpoint_interval_);
    DoBackupCheckpoint(jcr);
  }
}

}  // namespace storagedaemon
