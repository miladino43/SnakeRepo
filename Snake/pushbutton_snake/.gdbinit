# Load the PetaLinux SDK main gdbinit script
shell echo source ${PETALINUX}/etc/gdbinit > /tmp/gdbinit
source /tmp/gdbinit
shell rm /tmp/gdbinit

