# pg_auto_tune
Utility to auto tune the PostgreSQL configuration parameters

# Usage

```
$ ./pg_auto_tune --help                                           
pg_auto_tune - Auto tuning for PostgreSQL by Percona
Usage:
pg_auto_tune [OPTION...] [data-dir]
Options:
  -h, --host-type=TYPE        TYPE can be "pod", "standard", or "cloud"
  -n, --node-type=TYPE        TYPE can be "primary", or "standby"
  -d, --disk-type=TYPE        TYPE can be "magnetic", "ssd", or "network"
  -w, --workload-type=TYPE    TYPE can be "olap", "oltp", or "mixed" DEFAULE=[MIXED]
  -m, --file=file-path        path of config map file. DEFAULT:"ConfigMap.json"
  -o, --file=file-path        output conf file path. DEFAULT:"per_postgresql.conf"
  -D, --data-dir=DIR          location of the PostgreSQL data directory
  -F, --force-profile         Force apply invalid profiles. DEFAULT=[FALSE]
  -v, --verbose               output verbose messages
  -V, --version               output version information and exit
  -?, --help                  print this help


```

# Tuning Profiles
pg_auto_tune supports the tuning profiles in JSON format.
see https://github.com/codeforall/pg_auto_tune/tree/main/profiles for sample profiles.

# Supported platform
pg_auto_tune is only tested on Linux systems

# Build
```
$ make
```
command to build it

