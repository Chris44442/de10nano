
set masters [get_service_paths master]
set master [lindex $masters 0]
open_service master $master
master_write_32 $master 0x1000 0xaffe1234
set data [master_read_32 $master 0x1000 1]
puts $data
