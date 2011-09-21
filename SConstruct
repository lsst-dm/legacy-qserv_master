# -*- python -*-
#
# SConstruct for qserv/master
#
#  
# Helpful envvars:
#
# SEARCH_ROOTS : Set the include/lib search path prefixes.
#                Redhat/Fedora users will want to add "/usr" so that
#                /usr/lib64 is added.  Defaults to "/usr:/usr/local"
# MYSQL_ROOT : Search a different root for MySQL. Useful if you have a
#              MySQL build that isn't covered by SEARCH_ROOTS or if
#              you'd like to override the version(s) found in system
#              directories or SEARCH_ROOTS 
# SWIG : The path to the swig binary to use.  Use this to specify a
#        particular SWIG installation (1.3.29 is known to be faulty,
#        and is shipped with RHEL5)
#
# Xrootd-specific variables:  The builder expects a xrootd source tree
# compiled/built in-place, so the following XRD variables need to be
# specified since the tree cannot be found automatically.
# XRD_DIR : The path to the xrootd code checkout/clone (the top-level
#           xrootd directory that holds src/, lib/, configure.classic,
#           and others) 
# XRD_PLATFORM : The platform id used by xrootd for the
#                os/machine. e.g., x86_64_linux_26_dbg.  The builder
#                will check $XRD_DIR/lib/$XRD_PLATFORM for xrootd
#                libraries. 


import glob, os.path, re, sys
import distutils.sysconfig


### Distribution creation
def makePythonDist():
    print 'dist', distutils.sysconfig.get_python_inc()
    
if 'install' in COMMAND_LINE_TARGETS:
    makePythonDist()

# Xrootd/Scalla search helper
class XrdHelper:
    def __init__(self, roots):
        self.cands = roots
        if os.environ.has_key('XRD_DIR'):
            self.cands.insert(0, os.environ['XRD_DIR'])

        self.platforms = ["x86_64_linux_26","i386_linux26","i386_linux26_dbg"]
        if os.environ.has_key('XRD_PLATFORM'):
            self.platforms.insert(0, os.environ['XRD_PLATFORM'])
        pass

    def getXrdLibInc(self):
        for c in self.cands:
            (inc, lib) = (self._findXrdInc(c), self._findXrdLib(c))
            if inc and lib:
                return (inc, lib)
        return (None, None)

    def _findXrdLib(self, path):
        for p in self.platforms:
            libpath = os.path.join(path, "lib", p)
            if os.path.exists(libpath):
                return libpath
        return None

    def _findXrdInc(self, path):
        paths = map(lambda p: os.path.join(path, p), ["include/xrootd", "src"])
        for p in paths:
            neededFile = os.path.join(p, "XrdPosix/XrdPosix.hh")
            if os.path.exists(neededFile):
                return p
        return None
    pass

## Boost checker
def checkBoost(lib):
    if not conf.CheckLib(lib + "-gcc34-mt", language="C++") \
            and not conf.CheckLib(lib + "-gcc41-mt", language="C++") \
            and not conf.CheckLib(lib, language="C++") \
            and not conf.CheckLib(lib + "-mt", language="C++"):
        print >>sys.stderr, "Can't find " + lib
        canBuild = False

def composeEnv(env, roots=[], includes=[], libs=[]):
    env.Append(CPPPATH=[os.path.join(x, "include") for x in roots])
    env.Append(CPPPATH=includes)
    env.Append(LIBPATH=[os.path.join(x, "lib") for x in roots])
    env.Append(LIBPATH=libs)
    return env


##############################
## Work starts here.
##############################
env = Environment()
searchRoots = ['/usr', '/usr/local/'] # search in /usr/local by default.
if os.environ.has_key('SEARCH_ROOTS'):
    searchRoots = os.environ['SEARCH_ROOTS'].split(":")


# Start checking deps
# -------------------
hasXrootd = True
canBuild = True

# Find Scalla/xrootd directories
x = XrdHelper(searchRoots)
(xrd_inc, xrd_lib) = x.getXrdLibInc()
if (not xrd_inc) or (not xrd_lib):
    print >>sys.stderr, "Can't find xrootd headers or libraries"
    hasXrootd = False
else:
    print >> sys.stderr, "Using xrootd inc/lib: ", xrd_inc, xrd_lib

# Build the environment
env = Environment()
env.Tool('swig')
env['SWIGFLAGS'] = ['-python', '-c++', '-Iinclude'],
env['CPPPATH'] = [distutils.sysconfig.get_python_inc(), 'include'],
env['SHLIBPREFIX'] = ""

## Allow user-specified swig tool
## SWIG 1.3.29 has bugs which cause the build to fail.
## 1.3.36 is known to work.
if os.environ.has_key('SWIG'):
    env['SWIG'] = os.environ['SWIG']

searchLibs = [xrd_lib]
mysqlRoots = filter(lambda r: "mysql" in r, searchRoots)
if os.environ.has_key('MYSQL_ROOT'):
    mysqlRoots.append(os.environ['MYSQL_ROOT'])

if not mysqlRoots: # If not overridden, use general search roots.
    mysqlRoots = searchRoots

# Look for mysql sub-directories. lib64 is important on RH/Fedora
searchLibs += filter(os.path.exists, 
                     [os.path.join(r, lb, "mysql") 
                      for r in mysqlRoots for lb in ["lib","lib64"]])

composeEnv(env, roots=searchRoots, includes=[xrd_inc], libs=searchLibs)
if hasXrootd:
    env.Append(CPPPATH = [xrd_inc])
    env.Append(LIBPATH = [xrd_lib])
env.Append(CPPFLAGS = ["-D_LARGEFILE_SOURCE",
                       "-D_LARGEFILE64_SOURCE",
                       "-D_FILE_OFFSET_BITS=64",
                       "-D_REENTRANT",
                       "-g",
                       "-pedantic", "-Wno-long-long", "-Wall"])

# Start configuration tests
conf = Configure(env)

# boost library reqs
checkBoost("boost_thread")
checkBoost("boost_regex")

# ANTLR
if not conf.CheckCXXHeader("antlr/AST.hpp"):
    print >> sys.stderr, "Could not locate ANTLR headers"
    canBuild = False
if not conf.CheckLib("antlr", language="C++"):
    print >> sys.stderr, "Could not find ANTLR lib"
    canBuild = False

# Close out configuration for no parse test env
parseEnv = conf.Finish()
parseEnv = parseEnv.Clone(LIBS=parseEnv["LIBS"][:])
env = parseEnv.Clone()
conf = Configure(env)

# libssl
if not conf.CheckLib("ssl"):
    print >> sys.stderr, "Could not locate ssl"
    canBuild = False

# MySQL
if not conf.CheckCXXHeader("mysql/mysql.h"):
    print >> sys.stderr, "Could not locate MySQL headers"
    canBuild = False
if not conf.CheckLib("mysqlclient_r", language="C++"):
    print >> sys.stderr, "Could not find multithreaded mysql(mysqlclient_r)"
    canBuild = False

# XrdPosix library
if not conf.CheckLib("XrdPosix", language="C++"):
    print >>sys.stderr, "Can't use XrdPosix lib"
    hasXrootd = False

env = conf.Finish()    
canBuild = canBuild and hasXrootd

parserSrcs = map(lambda x: os.path.join('src', x), 
                 ["AggregateMgr.cc", 
                  "SqlParseRunner.cc",
                  "Templater.cc",
                  "parseTreeUtil.cc",
                  "parseHandlers.cc",
                  "dbgParse.cc",
                  "SqlSubstitution.cc",
                  "Substitution.cc",
                  "ChunkMapping.cc",
                  "SpatialUdfHandler.cc",
                  "SqlSQL2Lexer.cpp", "SqlSQL2Parser.cpp"] )
             

pyPath = 'python/lsst/qserv/master'
pyLib = os.path.join(pyPath, '_masterLib.so')
dispatchSrcs = map(lambda x: os.path.join('src', x), 
                   ["xrdfile.cc", 
                    "thread.cc", 
                    "MmapFile.cc",
                    "TableMerger.cc",
                    "SqlInsertIter.cc",
                    "PacketIter.cc",
                    "sql.cc",
                    "dispatcher.cc", "xrootd.cc",
                    "AsyncQueryManager.cc",
                    "ChunkQuery.cc",
                    "WorkQueue.cc",
                    "Timer.cc"])


srcPaths = dispatchSrcs + [os.path.join(pyPath, 'masterLib.i')] + parserSrcs


runTrans = { 'bin' : os.path.join('bin', 'runTransactions'),
             'srcPaths' : dispatchSrcs + [os.path.join('src',
                                                       "runTransactions.cc")]
             }
# Lexer and Parser cpp files should have been generated with
# "antlr -glib DmlSQL2.g SqlSQL2.g"
testParser = { 'bin' : os.path.join('bin', 'testCppParser'),
               'srcPaths' : (parserSrcs +
                             [os.path.join("tests","testCppParser.cc")]),
               }
# testIter doesn't need all dispatchSrcs, but it's not worth optimizing.
testIter = { 'bin' : os.path.join('bin', 'testIter'),
               'srcPaths' : ( map(lambda x: os.path.join('src', x), 
                                  ["SqlInsertIter.cc",
                                   "PacketIter.cc",
                                   "Timer.cc",
                                   "xrdfile.cc"]) +
                              [os.path.join("tests","testIter.cc")])}

xrdPrecache = { 'bin' : os.path.join('bin', 'xrdPrecache'),
                'srcPaths' : map(lambda x: os.path.join('src', x), 
                                 ["xrdfile.cc", 
                                  "xrootd.cc", 
                                  "WorkQueue.cc",
                                  "xrdPrecache.cc"])}
               

if canBuild:
    env.SharedLibrary(pyLib, srcPaths)
    env.Program(runTrans['bin'], runTrans["srcPaths"])
#    env.Program(target=testIter['bin'], 
 #               source=testIter["srcPaths"],
                #                LINKFLAGS='--static',
  #              LIBS=["XrdPosix", "XrdClient", "XrdSys", 
   #                   "XrdNet", "XrdOuc", "XrdOss", "crypto", "dl",
    #                  "boost_thread", "boost_regex", "pthread"])
#    parseEnv.Program(testParser['bin'], testParser["srcPaths"])
#    env.Program(target=xrdPrecache['bin'], source=xrdPrecache["srcPaths"],
#                LINKFLAGS='--static', 
#                LIBS=["XrdPosix", "XrdClient", "XrdSys", 
#                      "XrdNet", "XrdOuc", "XrdOss", "ssl", "crypto", "dl",
#                      "boost_thread", "pthread"])

# Describe what your package contains here.
env.Help("""
LSST Query Services master server package
""")

#
# Build/install things
#
for d in Split("lib examples doc"):
    if os.path.isdir(d):
        try:
            SConscript(os.path.join(d, "SConscript"), exports='env')
        except Exception, e:
            print >> sys.stderr, "%s: %s" % (os.path.join(d, "SConscript"), e)

if not canBuild:
    print >>sys.stderr, "****** Fatal errors. Didn't build anything. ******"
    

# env['IgnoreFiles'] = r"(~$|\.pyc$|^\.svn$|\.o$)"




# Alias("install", [env.Install(env['prefix'], "python"),
#                   env.Install(env['prefix'], "include"),
#                   env.Install(env['prefix'], "lib"),
#                   env.InstallAs(os.path.join(env['prefix'], "doc", "doxygen"),
#                                 os.path.join("doc", "htmlDir")),
#                   env.InstallEups(os.path.join(env['prefix'], "ups"))])

# scons.CleanTree(r"*~ core *.so *.os *.o")

# #
# # Build TAGS files
# #
# files = scons.filesToTag()
# if files:
#     env.Command("TAGS", files, "etags -o $TARGET $SOURCES")

# env.Declare()
