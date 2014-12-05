# Print glog messages
export DYLD_LIBRARY_PATH=$PWD/prefix/lib # OS X
export LD_LIBRARY_PATH=$PWD/prefix/lib   # Linux
export GLOG_logtostderr=1
export GLOG_colorlogtostderr=1
