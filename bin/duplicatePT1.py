#!/usr/bin/env python

# 
# LSST Data Management System
# Copyright 2008, 2009, 2010 LSST Corporation.
# 
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the LSST License Statement and 
# the GNU General Public License along with this program.  If not, 
# see <http://www.lsstcorp.org/LegalNotices/>.
#

# Data duplicator - duplicates a PT1 tile so that overall data set size
# is large enough for meaningful scalability testing.
# 
# Approach:
# For each table row, for the following columns, transform with a 
# simple function of f: old, raOff, declOff, copyNum -> new
#
# ranges for PT1 imsim data:
#   scienceCcdExposureId: 43729169930 -> 43914280686
#   rawAmpExposureId: 1399333437760 -> 1400862560911 
#   objectId: 396180668284928 -> 439529773203912
#   sourceId:  2865834880532484 -> 2877966299037945 
#   ra:  -5.203 -> 5.222
#   decl:  -4.34239174635 ->  4.725055533 
#
# 

import optparse
import sys

class OptionParser(optparse.OptionParser):
    def __init__(self):
        OptionParser.__init__(self) 
        return
        self.add_option("--offset", action="store_true", 
                      dest="appHelp", default=False, 
                      help="Get qserv frontend help.")
        self.add_option("-c", "--config", dest="configFile", default=None,
                          help="Use config file. Can also be specified with\n" +
                          "%s as an environment variable." % config.envFilenameVar)
        self.add_option("--check",
                          dest="checkTaskId", default=None, metavar="ID", 
                          help="Check status of task with id ID.")
        self.add_option("-q", "--query", 
                          dest="queryFile", default=None, metavar="FILENAME",
                          help="Invoke a query stored in FILENAME.\n" +
                          "'-' will read from standard input.")
        self.add_option("--hintfile", 
                          dest="queryHints", default=None, metavar="FILENAME",
                          help="Use FILENAME to get hints. Use with --query.")
        
        self.add_option("--hintTest",
                          dest="hintTest", default=None, metavar="NAME", 
                          help="Run a hintedParser test named NAME.")
        self.add_option("--test",
                          dest="testName", default=None, metavar="NAME", 
                          help="Run a hintedParser test named NAME.")
    pass

pt1ObjectColumns = ['objectId', 'iauId', 'ra_PS', 'ra_PS_Sigma', 'decl_PS', 'decl_PS_Sigma', 'radecl_PS_Cov', 'ra_SG', 'ra_SG_Sigma', 'decl_SG', 'decl_SG_Sigma', 'radecl_SG_Cov', 'raRange', 'declRange', 'muRa_PS', 'muRa_PS_Sigma', 'muDecl_PS', 'muDecl_PS_Sigma', 'muRaDecl_PS_Cov', 'parallax_PS', 'parallax_PS_Sigma', 'canonicalFilterId', 'extendedness', 'varProb', 'earliestObsTime', 'latestObsTime', 'flags', 'uNumObs', 'uExtendedness', 'uVarProb', 'uRaOffset_PS', 'uRaOffset_PS_Sigma', 'uDeclOffset_PS', 'uDeclOffset_PS_Sigma', 'uRaDeclOffset_PS_Cov', 'uRaOffset_SG', 'uRaOffset_SG_Sigma', 'uDeclOffset_SG', 'uDeclOffset_SG_Sigma', 'uRaDeclOffset_SG_Cov', 'uLnL_PS', 'uLnL_SG', 'uFlux_PS', 'uFlux_PS_Sigma', 'uFlux_SG', 'uFlux_SG_Sigma', 'uFlux_CSG', 'uFlux_CSG_Sigma', 'uTimescale', 'uEarliestObsTime', 'uLatestObsTime', 'uSersicN_SG', 'uSersicN_SG_Sigma', 'uE1_SG', 'uE1_SG_Sigma', 'uE2_SG', 'uE2_SG_Sigma', 'uRadius_SG', 'uRadius_SG_Sigma', 'uFlags', 'gNumObs', 'gExtendedness', 'gVarProb', 'gRaOffset_PS', 'gRaOffset_PS_Sigma', 'gDeclOffset_PS', 'gDeclOffset_PS_Sigma', 'gRaDeclOffset_PS_Cov', 'gRaOffset_SG', 'gRaOffset_SG_Sigma', 'gDeclOffset_SG', 'gDeclOffset_SG_Sigma', 'gRaDeclOffset_SG_Cov', 'gLnL_PS', 'gLnL_SG', 'gFlux_PS', 'gFlux_PS_Sigma', 'gFlux_SG', 'gFlux_SG_Sigma', 'gFlux_CSG', 'gFlux_CSG_Sigma', 'gTimescale', 'gEarliestObsTime', 'gLatestObsTime', 'gSersicN_SG', 'gSersicN_SG_Sigma', 'gE1_SG', 'gE1_SG_Sigma', 'gE2_SG', 'gE2_SG_Sigma', 'gRadius_SG', 'gRadius_SG_Sigma', 'gFlags', 'rNumObs', 'rExtendedness', 'rVarProb', 'rRaOffset_PS', 'rRaOffset_PS_Sigma', 'rDeclOffset_PS', 'rDeclOffset_PS_Sigma', 'rRaDeclOffset_PS_Cov', 'rRaOffset_SG', 'rRaOffset_SG_Sigma', 'rDeclOffset_SG', 'rDeclOffset_SG_Sigma', 'rRaDeclOffset_SG_Cov', 'rLnL_PS', 'rLnL_SG', 'rFlux_PS', 'rFlux_PS_Sigma', 'rFlux_SG', 'rFlux_SG_Sigma', 'rFlux_CSG', 'rFlux_CSG_Sigma', 'rTimescale', 'rEarliestObsTime', 'rLatestObsTime', 'rSersicN_SG', 'rSersicN_SG_Sigma', 'rE1_SG', 'rE1_SG_Sigma', 'rE2_SG', 'rE2_SG_Sigma', 'rRadius_SG', 'rRadius_SG_Sigma', 'rFlags', 'iNumObs', 'iExtendedness', 'iVarProb', 'iRaOffset_PS', 'iRaOffset_PS_Sigma', 'iDeclOffset_PS', 'iDeclOffset_PS_Sigma', 'iRaDeclOffset_PS_Cov', 'iRaOffset_SG', 'iRaOffset_SG_Sigma', 'iDeclOffset_SG', 'iDeclOffset_SG_Sigma', 'iRaDeclOffset_SG_Cov', 'iLnL_PS', 'iLnL_SG', 'iFlux_PS', 'iFlux_PS_Sigma', 'iFlux_SG', 'iFlux_SG_Sigma', 'iFlux_CSG', 'iFlux_CSG_Sigma', 'iTimescale', 'iEarliestObsTime', 'iLatestObsTime', 'iSersicN_SG', 'iSersicN_SG_Sigma', 'iE1_SG', 'iE1_SG_Sigma', 'iE2_SG', 'iE2_SG_Sigma', 'iRadius_SG', 'iRadius_SG_Sigma', 'iFlags', 'zNumObs', 'zExtendedness', 'zVarProb', 'zRaOffset_PS', 'zRaOffset_PS_Sigma', 'zDeclOffset_PS', 'zDeclOffset_PS_Sigma', 'zRaDeclOffset_PS_Cov', 'zRaOffset_SG', 'zRaOffset_SG_Sigma', 'zDeclOffset_SG', 'zDeclOffset_SG_Sigma', 'zRaDeclOffset_SG_Cov', 'zLnL_PS', 'zLnL_SG', 'zFlux_PS', 'zFlux_PS_Sigma', 'zFlux_SG', 'zFlux_SG_Sigma', 'zFlux_CSG', 'zFlux_CSG_Sigma', 'zTimescale', 'zEarliestObsTime', 'zLatestObsTime', 'zSersicN_SG', 'zSersicN_SG_Sigma', 'zE1_SG', 'zE1_SG_Sigma', 'zE2_SG', 'zE2_SG_Sigma', 'zRadius_SG', 'zRadius_SG_Sigma', 'zFlags', 'yNumObs', 'yExtendedness', 'yVarProb', 'yRaOffset_PS', 'yRaOffset_PS_Sigma', 'yDeclOffset_PS', 'yDeclOffset_PS_Sigma', 'yRaDeclOffset_PS_Cov', 'yRaOffset_SG', 'yRaOffset_SG_Sigma', 'yDeclOffset_SG', 'yDeclOffset_SG_Sigma', 'yRaDeclOffset_SG_Cov', 'yLnL_PS', 'yLnL_SG', 'yFlux_PS', 'yFlux_PS_Sigma', 'yFlux_SG', 'yFlux_SG_Sigma', 'yFlux_CSG', 'yFlux_CSG_Sigma', 'yTimescale', 'yEarliestObsTime', 'yLatestObsTime', 'ySersicN_SG', 'ySersicN_SG_Sigma', 'yE1_SG', 'yE1_SG_Sigma', 'yE2_SG', 'yE2_SG_Sigma', 'yRadius_SG', 'yRadius_SG_Sigma', 'yFlags']
pt1SourceColumns = ['sourceId', 'scienceCcdExposureId', 'filterId', 'objectId', 'movingObjectId', 'procHistoryId', 'ra', 'raErrForDetection', 'raErrForWcs', 'decl', 'declErrForDetection', 'declErrForWcs', 'xFlux', 'xFluxErr', 'yFlux', 'yFluxErr', 'raFlux', 'raFluxErr', 'declFlux', 'declFluxErr', 'xPeak', 'yPeak', 'raPeak', 'declPeak', 'xAstrom', 'xAstromErr', 'yAstrom', 'yAstromErr', 'raAstrom', 'raAstromErr', 'declAstrom', 'declAstromErr', 'raObject', 'declObject', 'taiMidPoint', 'taiRange', 'psfFlux', 'psfFluxErr', 'apFlux', 'apFluxErr', 'modelFlux', 'modelFluxErr', 'petroFlux', 'petroFluxErr', 'instFlux', 'instFluxErr', 'nonGrayCorrFlux', 'nonGrayCorrFluxErr', 'atmCorrFlux', 'atmCorrFluxErr', 'apDia', 'Ixx', 'IxxErr', 'Iyy', 'IyyErr', 'Ixy', 'IxyErr', 'snr', 'chi2', 'sky', 'skyErr', 'flagForAssociation', 'flagForDetection', 'flagForWcs']


def copyParamGen(originalBox, raCopies=2**30, declCopies=2**30, 
                 copyStart=-1, copyEnd=2**30):
    noZero = False
    (oRaMin, oDeclMin, oRaMax, oDeclMax) = originalBox
    raWidth = oRaMax - oRaMin
    declHeight = oDeclMax - oDeclMin
    raOffCount = int(360/raWidth)
    declOffCount = int(180/declHeight)

    def clip(bounds, origs, increment, candList):
        return filter(lambda off: (((origs[0] + off) >= bounds[0]) 
                                   and ((origs[1] + off) <= bounds[1])), 
                    map(lambda mul: increment * mul, candList))
    def trimByCopies(copies, candList):
        trimSize = len(candList) - copies
        if trimSize <= 0:
            return candList
        half = trimSize / 2
        if trimSize % 2:
            return candList[half+1 : -half]
        return candList[half : -half]

    raList = trimByCopies(raCopies, clip([-180, 180], [oRaMin, oRaMax], 
                                         raWidth, 
                                         range(-raOffCount, 
                                                raOffCount+1)))
    declList = trimByCopies(declCopies, clip([-90, 90], [oDeclMin, oDeclMax], 
                                             declHeight, 
                                             range(-declOffCount, 
                                                    declOffCount+1)))
    
    copyNum = 0
    # There are redundant iterations, but it shouldn't be a speed problem
    for declOff in raList:
        for raOff in declList:
            if noZero and (declOff == 0.0) and (raOff == 0.0):
                continue
            if (copyNum >= copyStart) and (copyNum < copyEnd):
                print "Yielding",raOff,declOff,copyNum
                yield (raOff, declOff, copyNum)
            copyNum += 1 
    pass

class GroupWriter:    
    def __init__(self, copyGen=None, filePrefix="group_", headerRow=pt1ObjectColumns):
        self.writers = []
        if copyGen:
            for p in copyGen:
                self.writers.append(Writer(p, filePrefix + str(p[2]), 
                                           headerRow))
        pass
    def write(self, row):
        map(lambda w: w.write(row), self.writers)
    pass

class Writer:
    def __init__(self, copyParam, csvFile, headerRow):
        (raOff, declOff, copyNum) = copyParam
        self.w = open(csvFile, "w")
        self.headerDict = dict(zip(headerRow, range(len(headerRow))))
        # Complete transformation guide

        raFunc = lambda old: str(float(old) + raOff)
        declFunc = lambda old: str(float(old) + declOff)
        skytileRange = 194400
        self.columnMap = {
            "scienceCcdExposureId" : lambda old: str((copyNum << 36) + int(old)),
            "rawAmpExposureId" : lambda old: str((copyNum << 41) + int(old)),
            "sourceId" : lambda old: str((copyNum << 44) + int(old)),
            "objectId" : lambda old: str(((copyNum*skytileRange) << 32) + int(old)),
            "snapCcdExposureId" : lambda old: str((copyNum << 38) + int(old)),
            }
        # Add ra and decl column functions
        for c in ['ra_PS',
                  'ra_SG',  
                  # raRange is a size, not a coordinate???
                  "ra",
                  # raFlux ???
                  "raPeak",
                  "raAstrom",
                  "raObject",
                  "crval1"
                  ]:
            self.columnMap[c] = raFunc
        for c in ['decl_PS',
                  'decl_SG',  
                  "decl",
                  "declPeak",
                  "declAstrom",
                  "declObject",
                  "crval2",
                  ]:
            self.columnMap[c] = declFunc
            
        self.transformMap = {}
        # Build transform map for this csv file.
        for c,f in self.columnMap.items():
            if c in self.headerDict:
                self.transformMap[self.headerDict[c]] = f
        pass

    def transform(self, row):
        newRow = row[:]
        for col,f in self.transformMap.items():
            newRow[col] = f(row[col])
        return newRow

    def write(self, row):
        self.w.write(",".join(self.transform(row))+"\n")
        pass
 
def csvRowGen(csvFile, delim):
    for x in open(csvFile, "r"):
        yield x.split(delim)

class HeaderReader:
    def __init__(self, f, delim):
        self.src = open(f, "r")
        self.headerRow = self.src.readline().rstrip().split(",")
    def gen(self):
        while True:
            r = self.src.readline()
            if len(r) == 0:
                return
            yield r.rstrip()
    
def testdrive(csvFile, delim):
    r = HeaderReader(csvFile, delim)
    originalBox = [0,0,10,10]
    raCopies = 4
    declCopies = 4
    w = GroupWriter(copyParamGen(originalBox, raCopies, declCopies, 
                                 copyStart=-1, copyEnd=2**30),
                    csvFile+"_c", r.headerRow)

    map(lambda l: w.write(l.split(delim)), r.gen())
    
def main():    
    #    p = OptionParser()
    #    (options, args) = p.parse_args()
    testdrive("dummy.csv", ",")
    return

if __name__ == '__main__':
    main()
