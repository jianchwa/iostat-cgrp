iostat-cgrp is based on 'iostat' policy of blk-rq-qos. It read from
the blkio.iostat of root or non root cgroups and output them in more
human friendly fashion. Such as,
<pre>
  iostat-cgrp -r -o rwDM vda vdb test
  ...
  Device DATA       IOPS         BW       RQSZ       QLAT       DLAT Cgroup
     vda    R    16.00/s 572.00KB/s     35.75K     9.46us   250.68us test
     vdb    W   249.00/s  50.34MB/s    207.02K   254.33us   137.41ms
  Device META       IOPS         BW       RQSZ       QLAT       DLAT Cgroup
     vdb    W    44.00/s 792.00KB/s     18.00K   191.20us   225.25ms
  Device DATA       IOPS         BW       RQSZ       QLAT       DLAT Cgroup
     vda    R    33.00/s 412.00KB/s     12.48K     8.49us   180.84us test
     vdb    W    65.00/s  12.71MB/s    200.31K   432.02us   335.31ms
     vdb    W    38.00/s  12.66MB/s    341.26K   135.56us   230.27ms test
  Device META       IOPS         BW       RQSZ       QLAT       DLAT Cgroup
     vda    R     5.00/s  68.00KB/s     13.60K    12.51us   162.52us test
     vdb    W   119.00/s   2.28MB/s     19.63K    10.40ms   149.88ms
  Device DATA       IOPS         BW       RQSZ       QLAT       DLAT Cgroup
     vda    R    20.00/s 232.00KB/s     11.60K     8.71us   514.30us test
     vdb    W   183.00/s  35.02MB/s    195.96K   196.82us   129.58ms
     vdb    W     1.00/s 380.00KB/s    380.00K    48.51us   552.68ms test
</pre>
Look at the commandline to use iostat-cgrp
<pre>
iostat-cgrp -r -o rwDM vda vdb test

-r  : Show IO statistics of root cgroup which doesn't include children cgroups.
      Hope to see the metadata performance of filesystem such as xfs

-o  : 'r' - read, 'w' - write, 'd' - discard (not support right now)
      'M' - meta data, 'D' - regular data

vda : Block device name; The user can specify mutiple bdev here.

test: Cgroup name you want to observe. It is the path name under /sys/fs/cgroup/blkio
      If you want to observe a process, just make a cgroup for it.
</pre>
