task configure, "initialize the build":
   mkdir "build"
   cd "build"
   exec "cmake .."
   setCommand "nop"

task build, "build the app":
   exec "ninja -C build"
   cd "main"
   switch "out", "../build/multilink"
   switch "nimcache", "../build/nimcache"
   switch "threads", "on"
   switch "passl", "-L../build -lappremote -lmultilink -lreactor -ljson11 -lstdc++ -lm"
   setCommand "c", "mlapp"
