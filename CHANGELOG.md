All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Storage -> Device `Count` behavior changes

If the `Count` directive is specified with a value > 1,
duplicated devices will be created starting from serial-number 0000 up to `Count`.
The 0000 device is automatically assigned 'Autoselect=No'. 
Additionally, an autochanger resource is created with the name of the device the `Count` directive is specified for.
The duplicated devices will be assigned to this autochanger unless they are used in another autochanger already.

See explanations and example in documentation https://docs.bareos.org/TasksAndConcepts/VolumeManagement.html#concurrent-disk-jobs

### Configuring a disk Autochanger
If you want to run multiple jobs in parallel to the same disk storage,
you can now simply specify the `count` directive in the (Storage -> Device) configuration
to the number you've specified in `MaximumConcurrentJobs` (Director -> Storage).

If you want to migrate from your manually configured disk autochanger to simply using the `count` directive:
1. remove the disk autochanger resource
2. have a storage device with a specified `count` directive (with value >1)
3. wherever you used the disk autochanger name before, use the name of the device from step 2.

### Removed
- config: deprecate file daemon as alias for client in FD config [PR #2187]

### Fixed
- cats: fix version.map.in [PR #2064]
- webui: fix for PHP < 7.3 [PR #2067]
- cmake: fix MARIADB_MYSQL_INSTALL_DB_SCRIPT usage [PR #2040]
- Fix building ndmjob program [PR #2079]
- bareos-config-libs: double quote dbconfig values [PR #2111]
- freebsd: automate creation of pkg-plist.database-postgresql [PR #2102]
- plugin: postgresql use integer for savepkt.object_index [PR #2132]
- bconsole: enable app icon on windows [PR #2105]
- windows: fix readlink buffer size issue [PR #2153]
- Fix btape fill-test problem [PR #2018]
- cmake: add cmake_parse_arguments policy CMP0174 [PR #2169]
- VMware plugin: fix check_mac_address() for vm.config not present [PR #2059]
- scheduler: 'last' keyword doesn't allow job to be visible in status dir [PR #2120]
- fix autodeflate messages and refactor setup method [PR #2121]
- stored: fix crash when using jit reservation with no matching device; fix reservation error [PR #2141]
- Fix 32bit compilation [PR #2175]
- config: fix issues with config directive aliases [PR #2159]
- cleanup: fix multi-config tests [PR #2202]
- mssqlvdi-fd: add support for filestream backups [PR #2072]
- cmake: fix not including BareosTargetTools in systemtests, needed for get_target_output_dir [PR #2232]
- fix MaximumConcurrentJobs in docs and defaultsconfig [PR #2225]
- cli-test: fix test on freebsd [PR #2241]
- core: refactor config parser; fix ktls configuration; fix crashes/ub [PR #2222]
- contrib add support for mariadb 11+ [PR #2215]
- mariabackup: reset self.mycnf to string type [PR #2252]
- dird: fix bugs in DateTime [PR #2260]
- config: fix Director -> Director resource [PR #2259]
- mtx-changer: make mandatory test mt-st versus cpio-mt [PR #2256]
- packaging: set all `*.conf.examples` as %config(noreplace) [PR #2268]
- disable bareosfd-python3-module-test on FreeBSD [PR #2278]

### Documentation
- docs: fix grpc-fd plugin call [PR #2068]
- docs: add devtools documentation [PR #2149]
- docs rear: update to rear >= 2.8 [PR #2220]

### Added
- added build-dep to libutfcpp-dev in debian-like environments [PR #2056]

### Changed
- plugins: Fix typo in postgresql plugin [PR #2066]
- Sync EvpDigest between OpenSSL <1.1 and 1.1+ [PR #2086]
- winbareos-native.nsi: do not package python3 plugins [PR #2076]
- logrotate: add mtx-changer debug log config [PR #2039]
- cmake: add cmake check whether tirpc is installed [PR #2109]
- bconsole: require only one password in the configuration [PR #2116]
- openssl: unify ssl error logging [PR #2078]
- Inherit RunScript elements between JobDef resources [PR #2097]
- python-bareos: Add missing dh-python build dep [PR #2130]
- stored: list all devices if device is invalid/missing [PR #2122]
- Refactor some Xattr and Acl internals [PR #1893]
- debian: Add missing build dependencies [PR #2128]
- systemtests: fail if daemons have config warnings [PR #2144]
- contrib: check_chunk.py improve README.md instructions [PR #2147]
- build: add support for el10 [PR #2106]
- VMware Plugin: Fix CBT query handling [PR #2152]
- increase accepted warnings for windows [PR #2191]
- webui: use TemporaryDir as userdatadir in Selenium test [PR #2194]
- ndmp: remove ndmp backup level limit [PR #2188]
- reduce test-matrix [PR #2192]
- bareos-fd: add option for grpc fallback [PR #2104]
- Make tests optional [PR #2180]
- windows: single-config/single-output directory [PR #2211]
- webui: use WEB_UI path in apache bareos-webui.conf file [PR #2201]
- config: update directives and error out on bconsole config warnings [PR #2217]
- config: add virtual file changer example + documentation [PR #2090]
- stored: fix sd volume limit [PR #2264]
- various cmake improvements [PR #2176]

[PR #1893]: https://github.com/bareos/bareos/pull/1893
[PR #2018]: https://github.com/bareos/bareos/pull/2018
[PR #2039]: https://github.com/bareos/bareos/pull/2039
[PR #2040]: https://github.com/bareos/bareos/pull/2040
[PR #2056]: https://github.com/bareos/bareos/pull/2056
[PR #2059]: https://github.com/bareos/bareos/pull/2059
[PR #2064]: https://github.com/bareos/bareos/pull/2064
[PR #2066]: https://github.com/bareos/bareos/pull/2066
[PR #2067]: https://github.com/bareos/bareos/pull/2067
[PR #2068]: https://github.com/bareos/bareos/pull/2068
[PR #2072]: https://github.com/bareos/bareos/pull/2072
[PR #2076]: https://github.com/bareos/bareos/pull/2076
[PR #2078]: https://github.com/bareos/bareos/pull/2078
[PR #2079]: https://github.com/bareos/bareos/pull/2079
[PR #2086]: https://github.com/bareos/bareos/pull/2086
[PR #2090]: https://github.com/bareos/bareos/pull/2090
[PR #2097]: https://github.com/bareos/bareos/pull/2097
[PR #2102]: https://github.com/bareos/bareos/pull/2102
[PR #2104]: https://github.com/bareos/bareos/pull/2104
[PR #2105]: https://github.com/bareos/bareos/pull/2105
[PR #2106]: https://github.com/bareos/bareos/pull/2106
[PR #2109]: https://github.com/bareos/bareos/pull/2109
[PR #2111]: https://github.com/bareos/bareos/pull/2111
[PR #2116]: https://github.com/bareos/bareos/pull/2116
[PR #2120]: https://github.com/bareos/bareos/pull/2120
[PR #2121]: https://github.com/bareos/bareos/pull/2121
[PR #2122]: https://github.com/bareos/bareos/pull/2122
[PR #2128]: https://github.com/bareos/bareos/pull/2128
[PR #2130]: https://github.com/bareos/bareos/pull/2130
[PR #2132]: https://github.com/bareos/bareos/pull/2132
[PR #2141]: https://github.com/bareos/bareos/pull/2141
[PR #2144]: https://github.com/bareos/bareos/pull/2144
[PR #2147]: https://github.com/bareos/bareos/pull/2147
[PR #2149]: https://github.com/bareos/bareos/pull/2149
[PR #2152]: https://github.com/bareos/bareos/pull/2152
[PR #2153]: https://github.com/bareos/bareos/pull/2153
[PR #2159]: https://github.com/bareos/bareos/pull/2159
[PR #2169]: https://github.com/bareos/bareos/pull/2169
[PR #2175]: https://github.com/bareos/bareos/pull/2175
[PR #2176]: https://github.com/bareos/bareos/pull/2176
[PR #2180]: https://github.com/bareos/bareos/pull/2180
[PR #2187]: https://github.com/bareos/bareos/pull/2187
[PR #2188]: https://github.com/bareos/bareos/pull/2188
[PR #2191]: https://github.com/bareos/bareos/pull/2191
[PR #2192]: https://github.com/bareos/bareos/pull/2192
[PR #2194]: https://github.com/bareos/bareos/pull/2194
[PR #2201]: https://github.com/bareos/bareos/pull/2201
[PR #2202]: https://github.com/bareos/bareos/pull/2202
[PR #2211]: https://github.com/bareos/bareos/pull/2211
[PR #2215]: https://github.com/bareos/bareos/pull/2215
[PR #2217]: https://github.com/bareos/bareos/pull/2217
[PR #2220]: https://github.com/bareos/bareos/pull/2220
[PR #2222]: https://github.com/bareos/bareos/pull/2222
[PR #2225]: https://github.com/bareos/bareos/pull/2225
[PR #2232]: https://github.com/bareos/bareos/pull/2232
[PR #2241]: https://github.com/bareos/bareos/pull/2241
[PR #2252]: https://github.com/bareos/bareos/pull/2252
[PR #2256]: https://github.com/bareos/bareos/pull/2256
[PR #2259]: https://github.com/bareos/bareos/pull/2259
[PR #2260]: https://github.com/bareos/bareos/pull/2260
[PR #2264]: https://github.com/bareos/bareos/pull/2264
[PR #2268]: https://github.com/bareos/bareos/pull/2268
[PR #2278]: https://github.com/bareos/bareos/pull/2278
[unreleased]: https://github.com/bareos/bareos/tree/master
