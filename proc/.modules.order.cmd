cmd_/home/changda/git/kernels/examples/proc/modules.order := {   echo /home/changda/git/kernels/examples/proc/proc.ko; :; } | awk '!x[$$0]++' - > /home/changda/git/kernels/examples/proc/modules.order
