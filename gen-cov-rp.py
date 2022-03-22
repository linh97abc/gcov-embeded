import os
import shlex
import subprocess
import logging
import re
import glob
import sys

logger = logging.getLogger('Coverage-tool')
logger.setLevel(logging.DEBUG)

GCOV_COVERAGE_DUMP_START = "GCOV_COVERAGE_DUMP_START"
GCOV_COVERAGE_DUMP_END = "GCOV_COVERAGE_DUMP_END"
FILE_START_INDICATOR = '*'
GCOV_DUMP_SEPARATOR	= '<'

class CoverageTool:
    """ Base class for every supported coverage tool
    """

    def __init__(self):
        self.gcov_tool = None
        self.base_dir = None
        self.gen_html = True

    @staticmethod
    def factory(tool):
        if tool == 'gcovr':
            t =  Gcovr()
        else:
            logger.error("Unsupported coverage tool specified: {}".format(tool))
            return None

        logger.debug(f"Select {tool} as the coverage tool...")
        return t

    @staticmethod
    def retrieve_gcov_data(input_file):
        logger.debug("Working on %s" % input_file)
        extracted_coverage_info = {}
        capture_data = False
        capture_complete = False
        with open(input_file, 'r') as fp:
            for line in fp.readlines():
                if re.search(GCOV_COVERAGE_DUMP_START, line):
                    capture_data = True
                    continue
                if re.search(GCOV_COVERAGE_DUMP_END, line):
                    capture_complete = True
                    break
                # Loop until the coverage data is found.
                if not capture_data:
                    continue
                if line.startswith(FILE_START_INDICATOR):
                    sp = line.split(GCOV_DUMP_SEPARATOR)
                    if len(sp) > 1:
                        # Remove the leading delimiter "*"
                        file_name = sp[0][1:]
                        # Remove the trailing new line char
                        hex_dump = sp[1][:-1]
                    else:
                        continue
                else:
                    continue
                extracted_coverage_info.update({file_name: hex_dump})
        if not capture_data:
            capture_complete = True
        return {'complete': capture_complete, 'data': extracted_coverage_info}

    @staticmethod
    def create_gcda_files(extracted_coverage_info):
        logger.debug("Generating gcda files")
        for filename, hexdump_val in extracted_coverage_info.items():
            # if kobject_hash is given for coverage gcovr fails
            # hence skipping it problem only in gcovr v4.1
            if "kobject_hash" in filename:
                filename = (filename[:-4]) + "gcno"
                try:
                    os.remove(filename)
                except Exception:
                    pass
                continue

            with open(filename, 'wb') as fp:
                fp.write(bytes.fromhex(hexdump_val))

    def generate(self, outdir):
        for filename in glob.glob("%s/**/handler.log" % outdir, recursive=True):
            gcov_data = self.__class__.retrieve_gcov_data(filename)
            capture_complete = gcov_data['complete']
            extracted_coverage_info = gcov_data['data']
            if capture_complete:
                self.__class__.create_gcda_files(extracted_coverage_info)
                logger.debug("Gcov data captured: {}".format(filename))
            else:
                logger.error("Gcov data capture incomplete: {}".format(filename))

        if self.gen_html:
            with open(os.path.join(outdir, "coverage.log"), "a") as coveragelog:
                ret = self._generate(outdir, coveragelog)
                if ret == 0:
                    logger.info("HTML report generated: {}".format(
                        os.path.join(outdir, "coverage", "index.html")))

class Gcovr(CoverageTool):

    def __init__(self):
        super().__init__()
        self.ignores = []

    def add_ignore_file(self, pattern):
        self.ignores.append('.*' + pattern + '.*')

    def add_ignore_directory(self, pattern):
        self.ignores.append(".*/" + pattern + '/.*')

    @staticmethod
    def _interleave_list(prefix, list):
        tuple_list = [(prefix, item) for item in list]
        return [item for sublist in tuple_list for item in sublist]

    def _generate(self, outdir, coveragelog):
        coveragefile = os.path.join(outdir, "coverage.json")

        excludes = Gcovr._interleave_list("-e", self.ignores)

        cmd = ["gcovr", "-r", self.base_dir, "--gcov-executable",
               self.gcov_tool] + excludes + ["--json", "-o",
               coveragefile, outdir]
        cmd_str = " ".join(cmd)
        logger.debug(f"Running {cmd_str}...")
        subprocess.call(cmd, stdout=coveragelog)

        files = [coveragefile]

        subdir = os.path.join(outdir, "coverage")
        os.makedirs(subdir, exist_ok=True)

        tracefiles = self._interleave_list("--add-tracefile", files)

        return subprocess.call(["gcovr", "-r", self.base_dir, "--html",
                                "--html-details"] + tracefiles +
                               ["-o", os.path.join(subdir, "index.html")],
                               stdout=coveragelog)


if __name__ == "__main__":
    gcov_tool = sys.argv[1].replace('\\', '/')
    coverage_basedir = sys.argv[2]
    outdir = sys.argv[3]
    
    fh = logging.FileHandler(os.path.join(outdir, "gen-cov-rp.log"))
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
    ch = logging.StreamHandler()
    ch.setFormatter(logging.Formatter('%(levelname)-7s - %(message)s'))
    logger.addHandler(ch)
    logger.addHandler(fh)

    cov_tool = 'gcovr'
    logger.info("Generating coverage files...")
    coverage_tool = CoverageTool.factory(cov_tool)

    cmd = shlex.split(gcov_tool)
    try:    
        subprocess.check_call(cmd, stdout= subprocess.PIPE, stderr=subprocess.PIPE)
    except(subprocess.CalledProcessError):
        pass
    except(FileNotFoundError):
        coverage_tool.gen_html = False
        logger.info("File not found: " + cmd[0])
        logger.warning("Not generate html report")

    coverage_tool.gcov_tool = gcov_tool
    coverage_tool.base_dir = os.path.abspath(coverage_basedir)
    # coverage_tool.add_ignore_file('generated')
    # coverage_tool.add_ignore_directory('tests')
    coverage_tool.generate(outdir)
