#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2014 Krzysztof Miesowicz (krzysztof.miesowicz@gmail.com)
# All rights reserved.
#
# This file is part of the RTEMS Tools package in 'rtems-tools'.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

from rtemstoolkit import path
from rtemstoolkit import log
from rtemstoolkit import execute
from rtemstoolkit import macros
import shutil
import os
from datetime import datetime

import options

class summary:
    def __init__(self, p_summary_dir):
        self.summary_file_path = path.join(p_summary_dir, "summary.txt")
        self.index_file_path = path.join(p_summary_dir, "index.html")
        self.bytes_analyzed = 0
        self.bytes_not_executed = 0
        self.percentage_executed = 0.0
        self.percentage_not_executed = 100.0
        self.ranges_uncovered = 0
        self.branches_uncovered = 0
        self.branches_total = 0
        self.branches_always_taken = 0
        self.branches_never_taken = 0
        self.percentage_branches_covered = 0.0
        self.is_failure = False

    def parse(self):
        if(not path.exists(self.summary_file_path)):
            log.warning("Summary file " + self.summary_file_path + " does not exist!")
            self.is_failure = True
            return

        summary_file = open(self.summary_file_path,'r')
        self.bytes_analyzed = self._get_value_from_next_line_with_colon(summary_file)
        self.bytes_not_executed = self._get_value_from_next_line_with_colon(summary_file)
        self.percentage_executed = self._get_value_from_next_line_with_colon(summary_file)
        self.percentage_not_executed = self._get_value_from_next_line_with_colon(summary_file)
        self.ranges_uncovered = self._get_value_from_next_line_with_colon(summary_file)
        self.branches_total = self._get_value_from_next_line_with_colon(summary_file)
        self.branches_uncovered = self._get_value_from_next_line_with_colon(summary_file)
        self.branches_always_taken = self._get_value_from_next_line_without_colon(summary_file)
        self.branches_never_taken = self._get_value_from_next_line_without_colon(summary_file)
        summary_file.close()
        if not self.branches_uncovered == '' and not self.branches_total == '':
            self.percentage_branches_covered = 1 - float(self.branches_uncovered) / float(self.branches_total)
        else:
            self.percentage_branches_covered = 0.0
        return

    def _get_value_from_next_line_with_colon(self, summary_file):
        line = summary_file.readline()
        if ':' in line:
            return line.split(':')[1].strip()
        else:
            return ''

    def _get_value_from_next_line_without_colon(self, summary_file):
        line = summary_file.readline()
        return line.strip().split(' ')[0]

class report_gen:
    def __init__(self, p_symbol_sets_list, p_target_dir):
        self.symbol_sets_list = p_symbol_sets_list
        self.target_dir = p_target_dir
        self.partial_reports_files = list(["index.html", "summary.txt"])
        self.number_of_columns = 1

    def _find_partial_reports(self):
        partial_reports = {}
        for symbol_set in self.symbol_sets_list:
            set_summary = summary(path.join(self.target_dir, "test", symbol_set))
            set_summary.parse()
            partial_reports[symbol_set] = set_summary
        return partial_reports

    def _prepare_head_section(self):
        head_section = '''
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
        return head_section

    def _prepare_index_content(self, partial_reports):
        header = "<h1> RTEMS coverage analysis report </h1>"
        header += "<h3>Coverage reports by symbols sets:</h3>"
        table = "<table>"
        table += self._header_row()
        for symbol_set in partial_reports:
            table += self._row(symbol_set, partial_reports[symbol_set])
        table += "</table> </br>"
        timestamp = "Analysis performed on " + datetime.now().ctime()
        return "<body>\n" + header + table + timestamp + "\n</body>"

    def _row(self, symbol_set, summary):
        row = "<tr>"
        row += "<td>" + symbol_set + "</td>"
        if summary.is_failure:
            row += ' <td colspan="' + str(self.number_of_columns-1) + '" style="background-color:red">FAILURE</td>'
        else:
            row += " <td>" + self._link(summary.index_file_path,"Index") + "</td>"
            row += " <td>" + self._link(summary.summary_file_path,"Summary") + "</td>"
            row += " <td>" + summary.bytes_analyzed + "</td>"
            row += " <td>" + summary.bytes_not_executed + "</td>"
            row += " <td>" + summary.ranges_uncovered + "</td>"
            row += " <td>" + summary.percentage_executed + "%</td>"
            row += " <td>" + summary.percentage_not_executed + "%</td>"
            row += ' <td><progress value="' + summary.percentage_executed + '" max="100"></progress></td>'
            row += " <td>" + summary.branches_uncovered + "</td>"
            row += " <td>" + summary.branches_total + "</td>"
            row += " <td> {:.3%} </td>".format(summary.percentage_branches_covered)
            row += ' <td><progress value="{:.3}" max="100"></progress></td>'.format(100*summary.percentage_branches_covered)
            row += "</tr>\n"
        return row

    def _header_row(self):
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
        self.number_of_columns = row.count('<th>')
        return row

    def _link(self, address, text):
        return '<a href="' + address + '">' + text + '</a>'

    def _create_index_file(self, head_section, content):
        f = open(path.join(self.target_dir, "report.html"),"w")
        try:
            f.write(head_section)
            f.write(content)
        finally:
            f.close()

    def generate(self):
        partial_reports = self._find_partial_reports()
        head_section = self._prepare_head_section()
        index_content = self._prepare_index_content(partial_reports)
        self._create_index_file(head_section,index_content)
#         _create_summary_file(summary_content)

class symbols_configuration(object):
    '''
    Manages symbols configuration - reading from symbol file
    '''
    def __init__(self):
        self.symbol_sets = []

    def _log_invalid_format(self):
        log.stderr("Invalid symbol configuration file")
        log.stderr(''' Configuration file format:
                symbolset:
                   name=SYMBOLSET_NAME
                   lib=PATH_TO_LIBRARY_1
                   lib=PATH_TO_LIBRARY_2
                symbolset:
                    name=SYMBOLSET_NAME_N
                    lib=PATH_TO_LIBRARY        ''')

    def load(self, symbol_set_config_file, path_to_builddir):
        scf = open(symbol_set_config_file, 'r')
        for line in scf:
            try:
                if line.strip().startswith("symbolset"):
                    self.symbol_sets.append(symbol_set("",[]))
                else:
                    splitted = line.split('=')
                    if(len(splitted) == 2):
                        key = splitted[0].strip()
                        value = splitted[1].strip()

                        if key == 'name':
                            self.symbol_sets[-1].name = value
                        elif key == 'lib':
                            lib = os.path.join(path_to_builddir, value)
                            log.stderr(lib + "\n")
                            self.symbol_sets[-1].libs.append(lib)
                        else:
                            log.stderr("Invalid key : " + key + " in symbol set configuration file " + symbol_set_config_file)
                    else:
                        self._log_invalid_format()
            except:
                self._log_invalid_format()
        scf.close()

    def save(self, path):
        scf = open(path, 'w')
        for sset in self.symbol_sets:
            sset.write_set_file(path)
        scf.close()

class symbol_set(object):
    def __init__(self, name, libs):
        self.name = name
        self.libs = libs

    def is_valid(self):
        if len(self.name) == 0:
            log.stderr("Invalid symbol set. Symbol set must have name! ")
            return False
        if len(self.libs) == 0:
            log.stderr("Invalid symbol set. Symbol set must have specified libraries!")
            return False
        for lib in self.libs:
            if not path.exists(lib):
                log.stderr("Invalid library path: " + lib)
                return False
        return True

    def write_set_file(self, path):
        f = open(path, 'w')
        f.write("symbolset:\n")
        f.write("\t name=" + self.name + '\n')
        for lib in self.libs:
            f.write("\t lib=" + lib + '\n')
        f.close()

class gcnos(object):
    def create_gcnos_file(self, gcnos_config_file_path, gcnos_file_path, path_to_build_dir):
        with open(gcnos_file_path, 'w') as gcnos_file:
            with open(gcnos_config_file_path, 'r') as config_file:
                for line in config_file:
                    if line.strip():
                        gcnos_file.write(path.join(path_to_build_dir, line))

class covoar(object):
    '''
    Covoar runner
    '''

    def __init__(self, base_result_dir, config_dir, traces_dir, covoar_src_dir):
        self.base_result_dir = base_result_dir
        self.config_dir = config_dir
        self.traces_dir = traces_dir
        self.covoar_src_dir = covoar_src_dir

    def run(self, set_name, covoar_config_file, symbol_file, gcnos_file):
        covoar_result_dir = path.join(self.base_result_dir, set_name)

        if (not path.exists(covoar_result_dir)):
            path.mkdir(covoar_result_dir)

        if (not path.exists(symbol_file)):
            log.stderr("Symbol set file: " + symbol_file + " doesn't exists! Covoar can not be run!")
            log.stderr("Skipping " + set_name)
            return

        command = "covoar -C" + covoar_config_file + " -S " + symbol_file + " -O " + covoar_result_dir + " " + path.join(self.traces_dir, "*.exe")
        if (path.exists(gcnos_file)):
            command = command + " -g " + gcnos_file
        log.notice("Running covoar for " + set_name, stdout_only=True)
        log.notice(command, stdout_only=True)
        executor = execute.execute(verbose=True, output=output_handler)
        exit_code = executor.shell(command, cwd=os.getcwd())
         
        shutil.copy2(path.join(self.covoar_src_dir, 'table.js'), path.join(covoar_result_dir, 'table.js'))
        shutil.copy2(path.join(self.covoar_src_dir, 'covoar.css'), path.join(covoar_result_dir, 'covoar.css'))
         
        log.notice("Coverage run for " + set_name + " finished ")
        status = "success"
        if (exit_code[0] != 0):
            status = "failure. Error code: " + str(exit_code[0])
        log.notice("Coverage run for " + set_name + " finished " + status)
        log.notice("-----------------------------------------------")

class coverage_run(object):
    '''
    Coverage analysis support for rtems-test
    '''

    def __init__(self, p_macros, path_to_builddir):
        '''
        Constructor
        '''
        self.macros = p_macros
        self.target_dir = self.macros['_cwd']
        self.test_dir = path.join(self.target_dir, "test")
        self.rtdir = path.abspath(self.macros['_rtdir'])
        self.rtscripts = self.macros.expand(self.macros['_rtscripts'])
        self.coverage_config_path = path.join(self.rtscripts, "coverage")
        self.symbol_config_path = path.join(self.coverage_config_path, "symbol_sets.cfg")
        self.traces_dir = path.join(self.target_dir, 'coverage')
        self.config_map = self.macros.macros['coverage']
        self.executables = None
        self.symbol_sets = []
        self.path_to_builddir= path_to_builddir
#        self.gcnos_file_path = path.join(self.coverage_config_path, "rtems.gcnos")

    def prepare_environment(self):
        if(path.exists(self.traces_dir)):
            path.removeall(self.traces_dir)
        path.mkdir(self.traces_dir)
        log.notice("Coverage environment prepared", stdout_only = True)

    def write_covoar_config(self, covoar_config_file):
        ccf = open(covoar_config_file, 'w')
        ccf.write("format = " + self.config_map['format'][2] + '\n')
        ccf.write("target = " + self.config_map['target'][2] + '\n')
        ccf.write("explanations = " + self.macros.expand(self.config_map['explanations'][2]) + '\n')
        ccf.write("coverageExtension = " + self.config_map['coverage_extension'][2] + '\n')
#        ccf.write("gcnosFile = " + self.macros.expand(self.config_map['gcnos_file'][2]) + '\n')
        ccf.write("executableExtension = " + self.config_map['executable_extension'][2] + '\n')
        ccf.write("projectName = " + self.config_map['project_name'][2] + '\n')
        ccf.close()

    def run(self):
        if self.executables == None:
            log.stderr("ERROR: Executables for coverage analysis unspecified!")
            raise Exception('Executable for coverage analysis unspecified')
        if self.config_map == None:
            log.stderr("ERROR: Configuration map for coverage analysis unspecified!")
            raise Exception("ERROR: Configuration map for coverage analysis unspecified!")

        covoar_config_file = path.join(self.traces_dir, 'config')
        self.write_covoar_config(covoar_config_file)
        if(not path.exists(covoar_config_file)):
            log.stderr("Covoar configuration file: " + path.abspath(covoar_config_file) + " doesn't exists! Covoar can not be run! ");
            return -1

        self._link_executables()

        symbol_config = symbols_configuration()
        symbol_config.load(self.symbol_config_path, self.path_to_builddir)
        # Create gcnos_configuration
        # load paths to gcno files, join paths with path_to_builddir
        # write gcnos file to traces dir
        gcnos_file = path.join(self.traces_dir, "rtems.gcnos") 
#        gcnos().create_gcnos_file(self.gcnos_file_path, gcnos_file, self.path_to_builddir)

        for sset in symbol_config.symbol_sets:
            if sset.is_valid():
                symbol_set_file = path.join(self.traces_dir, sset.name + ".symcfg")
                sset.write_set_file(symbol_set_file)
                self.symbol_sets.append(sset.name)

                covoar_run = covoar(self.test_dir, self.symbol_config_path, self.traces_dir, path.join(self.rtdir, 'covoar'))
                covoar_run.run(sset.name, covoar_config_file, symbol_set_file, gcnos_file)
            else:
                log.stderr("Invalid symbol set " + sset.name + ". Skipping covoar run.")

        self._generate_reports();
        self._cleanup();
        self._summarize();

    def _link_executables(self):
        log.notice("Linking executables to " + self.traces_dir)

        for exe in self.executables:
            dst = path.join(self.traces_dir, path.basename(exe))
            try:
                os.link(exe, dst)
            except OSError, e:
                log.stderr("creating hardlink from " + path.abspath(exe) + " to " + dst + " failed!")
                raise
        log.notice("Symlinks made")

    def _generate_reports(self):
        log.notice("Generating reports")
        report = report_gen(self.symbol_sets, self.target_dir)
        report.generate()

    def _cleanup(self):
        log.notice("Cleaning workspace up")
        #path.removeall(self.traces_dir)

    def _summarize(self):
        log.notice("Coverage analysis finished. You can find results in " + self.target_dir)

def output_handler(text):
    log.notice(text, stdout_only = False)

if __name__ == "__main__":
    print "coverage main"
    c = coverage_run("/home/hf/coverage_test_leon2","/home/hf/development/rtems/src/rtems-tools/tester")
    c.prepare_environment()
    c.executables = ["/home/hf/development/rtems/src/b-leon2/sparc-rtems4.11/c/leon2/testsuites/samples/hello/hello.ralf"]
    c.run()
