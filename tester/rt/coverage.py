'''
Created on Jul 7, 2014

@author: Krzysztof Mięsowicz <krzysztof.miesowicz@gmail.com>
'''
from rtemstoolkit import path
from rtemstoolkit import log
from rtemstoolkit import execute
from rtemstoolkit import macros
import os
from datetime import datetime

class summary:
    def __init__(self, p_summaryDir):
        self.summaryFilePath = path.join(p_summaryDir, "summary.txt")
        self.indexFilePath = path.join(p_summaryDir, "index.html")
        self.bytes_analyzed = 0
        self.bytes_notExecuted = 0
        self.percentage_executed = 0.0
        self.percentage_notExecuted = 100.0
        self.ranges_uncovered = 0
        self.branches_uncovered = 0
        self.branches_total = 0
        self.branches_alwaysTaken = 0
        self.branches_neverTaken = 0
        self.percentage_branchesCovered = 0.0

    def parse(self):
        if(not path.exists(self.summaryFilePath)):
            log.warning("Summary file " + self.summaryFilePath + " does not exist!")
            return

        summaryFile = open(self.summaryFilePath,'r')
        self.bytes_analyzed = self._getValueFromNextLineWithColon(summaryFile)
        self.bytes_notExecuted = self._getValueFromNextLineWithColon(summaryFile)
        self.percentage_executed = self._getValueFromNextLineWithColon(summaryFile)
        self.percentage_notExecuted = self._getValueFromNextLineWithColon(summaryFile)
        self.ranges_uncovered = self._getValueFromNextLineWithColon(summaryFile)
        self.branches_total = self._getValueFromNextLineWithColon(summaryFile)
        self.branches_uncovered = self._getValueFromNextLineWithColon(summaryFile)
        self.branches_alwaysTaken = self._getValueFromNextLineWithoutColon(summaryFile)
        self.branches_neverTaken = self._getValueFromNextLineWithoutColon(summaryFile)
        summaryFile.close()

        self.percentage_branchesCovered = 1 - float(self.branches_uncovered) / float(self.branches_total)
        return

    def _getValueFromNextLineWithColon(self, summaryFile):
        line = summaryFile.readline()
        return line.split(':')[1].strip()

    def _getValueFromNextLineWithoutColon(self, summaryFile):
        line = summaryFile.readline()
        return line.strip().split(' ')[0]

class reportGen:
    def __init__(self, p_symbolSetsList, p_targetDir):
        self.symbolSetsList = p_symbolSetsList
        self.targetDir = p_targetDir
        self.partialReportsFiles = list(["index.html", "summary.txt"])

    def _findPartialReports(self):
        partialReports = {}
        for symbolSet in self.symbolSetsList:
            setSummary = summary(path.join(self.targetDir, "test", symbolSet))
            setSummary.parse()
            partialReports[symbolSet] = setSummary
        return partialReports

    def _prepareHeadSection(self):
        headSection = '''
        <head>
        <title>RTEMS coverage report</title>
        <style type="text/css">
            progress[value] {
              -webkit-appearance: none;
               appearance: none;

              width: 150px;
              height: 15px;
            }
        </style>
        </head>'''
        return headSection

    def _prepareIndexContent(self, partialReports):
        header = "<h1> RTEMS coverage analysis report </h1>"
        header += "<h3>Coverage reports by symbols sets:</h3>"
        table = "<table>"
        table += self._headerRow()
        for symbolSet in partialReports:
            table += self._row(symbolSet, partialReports[symbolSet])
        table += "</table> </br>"
        timestamp = "Analysis performed on " + datetime.now().ctime()
        return "<body>\n" + header + table + timestamp + "\n</body>"

    def _row(self, symbolSet, summary):
        row = "<tr>"
        row += "<td>" + symbolSet + "</td>"
        row += " <td>" + self._link(summary.indexFilePath,"Index") + "</td>"
        row += " <td>" + self._link(summary.summaryFilePath,"Summary") + "</td>"
        row += " <td>" + summary.bytes_analyzed + "</td>"
        row += " <td>" + summary.bytes_notExecuted + "</td>"
        row += " <td>" + summary.ranges_uncovered + "</td>"
        row += " <td>" + summary.percentage_executed + "%</td>"
        row += " <td>" + summary.percentage_notExecuted + "%</td>"
        row += ' <td><progress value="' + summary.percentage_executed + '" max="100"></progress></td>'
        row += " <td>" + summary.branches_uncovered + "</td>"
        row += " <td>" + summary.branches_total + "</td>"
        row += " <td> {:.3%} </td>".format(summary.percentage_branchesCovered)
        row += ' <td><progress value="{:.3}" max="100"></progress></td>'.format(100*summary.percentage_branchesCovered)
        row += "</tr>\n"
        return row

    def _headerRow(self):
        row = "<tr>"
        row += "<th> Symbols set name </th>"
        row += "<th> Index file </th>"
        row += "<th> Summary file </th>"
        row += "<th> Bytes analyzed </th>"
        row += "<th> Bytes not executed </th>"
        row += "<th> Uncovered ranges </th>"
        row += "<th> Percentage covered </th>"
        row += "<th> Percentage uncovered </th>"
        row += "<th> Instruction coverage </th>"
        row += "<th> Branches uncovered </th>"
        row += "<th> Branches total </th>"
        row += "<th> Branches covered percentage </th>"
        row += "<th> Branches coverage </th>"
        row += "</tr>\n"
        return row

    def _link(self, address, text):
        return '<a href="' + address + '">' + text + '</a>'

    def _createIndexFile(self, headSection, content):
        f = open(path.join(self.targetDir, "report.html"),"w")
        try:
            f.write(headSection)
            f.write(content)
        finally:
            f.close()

    def generate(self):
        partialReports = self._findPartialReports()
        headSection = self._prepareHeadSection()
        indexContent = self._prepareIndexContent(partialReports)
        self._createIndexFile(headSection,indexContent)
#         _createSummaryFile(summaryContent)

class covoar(object):
    '''
    Covoar runner
    '''

    def __init__(self, baseResultDir, configDir, tracesDir):
        self.baseResultDir = baseResultDir
        self.configDir = configDir
        self.tracesDir = tracesDir

    def run(self, setName, covoarConfigFile, symbolFile):
        covoarResultDir = path.join(self.baseResultDir, setName)

        if (not path.exists(covoarResultDir)):
            path.mkdir(covoarResultDir)
        symbolFile = path.join(self.configDir, symbolFile)

        if (not path.exists(symbolFile)):
            log.stderr("Symbol file: " + symbolFile + " doesn't exists! Covoar can not be run!")
            log.stderr("Skipping " + setName)
            return

        command = "covoar -C" + covoarConfigFile + " -s " + symbolFile + " -O " + covoarResultDir + " " + path.join(self.tracesDir, "*.exe")
        log.notice("Running covoar for " + setName, stdout_only=True)
        log.notice(command, stdout_only=True)
        executor = execute.execute(verbose=True, output=output_handler)
        exit_code = executor.shell(command, cwd=os.getcwd())
        status = "success"
        if (exit_code != 0):
            status = "failure"
        log.notice("Coverage run for " + setName + " finished " + status)
        log.notice("-----------------------------------------------")

class coverage_run(object):
    '''
    Coverage analysis support for rtems-test
    '''

    def __init__(self, p_targetDir, p_rtdir):
        '''
        Constructor
        '''
        self.targetDir = p_targetDir
        self.testDir = path.join(self.targetDir, "test")
        self.covoarConfigPath = path.join(p_rtdir, "config")
        self.rtdir = path.abspath(p_rtdir)
        self.macros = macros.macros(name=path.join(self.rtdir,'rtems','testing','testing.mc'), rtdir=path.abspath(p_rtdir))
        self.rtscripts = self.macros.expand(self.macros['_rtscripts'])
        self.symbolConfigPath = path.join(self.rtscripts, "coverage")
        self.tracesDir = path.join(self.targetDir, 'coverage')
        self.executables = None
        self.symbolSets = []

    def prepareEnvironment(self):
        if(path.exists(self.tracesDir)):
            path.removeall(self.tracesDir)
        path.mkdir(self.tracesDir)
        log.notice("Coverage environment prepared", stdout_only = True)


    def run(self):
        if self.executables == None:
            log.stderr("ERROR: Executables for coverage analysis unspecified!")
            raise Exception('Executable for coverage analysis unspecified')

        covoarConfigFile = path.join(self.symbolConfigPath, 'config')
        if(not path.exists(covoarConfigFile)):
            log.stderr("Covoar configuration file: " + path.abspath(covoarConfigFile) + " doesn't exists! Covoar can not be run! ");
            return -1

        self._linkExecutables()

        symbolSetsFile = open(path.join(self.symbolConfigPath,"symbol_sets"), 'r');
        for symbolSet in symbolSetsFile:
            setName = symbolSet.split()[0]
            symbolFile = symbolSet.split()[1]
            self.symbolSets.append(setName)

            covoar_run = covoar(self.testDir, self.symbolConfigPath, self.tracesDir)
            covoar_run.run(setName, covoarConfigFile, symbolFile)

        self._generateReports();
        self._cleanup();
        self._summarize();

    def _linkExecutables(self):
        log.notice("Linking executables to " + self.tracesDir)

        for exe in self.executables:
            dst = path.join(self.tracesDir, path.basename(exe))
            log.notice("Making symlink from " + exe + " to " + dst)
            os.link(exe, dst)
        log.notice("Symlinks made")

    def _generateReports(self):
        log.notice("Generating reports")
        report = reportGen(self.symbolSets, self.targetDir)
        report.generate()

    def _cleanup(self):
        log.notice("Cleaning workspace up")
        path.removeall(self.tracesDir)

    def _summarize(self):
        log.notice("Coverage analysis finished. You can find results in " + self.targetDir)

def output_handler(text):
    log.notice(text, stdout_only = True)

if __name__ == "__main__":
    c = coverage_run("/home/rtems/coverage/test_new","/home/rtems/development/rtems/src/rtems-tools/tester")
    c.prepareEnvironment()
    c.executables = ["/home/rtems/development/rtems/src/b-pc386/i386-rtems4.11/c/pc386/testsuites/samples/hello/hello.exe"]
    c.run()
