import subprocess
import time
import sys
import datetime
import platform
import os
import threading
import functools
import unittest
import json
import xmlrunner
import shutil

def timeout(timeout):
    def deco(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            # res = [Exception('function [%s] timeout [%s seconds] exceeded!' % (func.__name__, timeout))]
            #res = [Exception('FAILED timeout [%s seconds] exceeded! ' % ( timeout))]
            globales = func.func_globals
            res = [Exception('FAILED timeout [%s seconds] exceeded! ' % (timeout))]
            def newFunc():
                try:
                    res[0] = func(*args, **kwargs)
                except ValueError:
                    res[0] = ValueError
            t = threading.Thread(target=newFunc)
            t.daemon = True
            try:
                t.start()
                t.join(timeout)
            except ValueError:
                print ('error starting thread')
                raise ValueError
            ret = res[0]
            if isinstance(ret, BaseException):
                pass # raise ret
            return ret
        return wrapper
    return deco



def read_line(proc, fd, end_string):
    data = ""
    new_byte = b''
    #t = time.time()
    while (new_byte != b'\n'):
        try:
            new_byte = fd.read(1)
            if new_byte == '' and proc.poll() != None:
                break
            elif new_byte:
                # data += new_byte
                data += str(new_byte) ##, encoding='utf-8')
                if data.endswith(end_string):
                    break;
            elif proc.poll() is not None:
                break
        except ValueError:
            # timeout occurred
            # print("read_line_timeout")
            break
    # print("read_line returned :"),
    # sys.stdout.write(data)
    return data

def read_til_getShell(proc, fd, text):
    globalvar.last_search = text
    globalvar.last_found=""
    data = []
    line = ""
    #t = time.time()
    # while line != text  and proc.poll() == None:
    while line.find(text,0,len(line))< 0  and proc.poll() == None and  globalvar.last_found.find(text,0,len(globalvar.last_found))< 0:
    #while line.find(text,0,len(line))< 0  and proc.poll() == None:
        try:
            line = read_line(proc, fd, text)
            globalvar.last_found = globalvar.last_found + line
            if line:
                data.append(line)
            elif proc.poll() is not None:
                break
        except ValueError:
            # timeout occurred
            print("read_line_timeout")
            break
    return "".join(data)

def kill_process(instance):
    results="PASS"
    home = os.path.expanduser("~")
    try:
        init_command = [MYSQL_SHELL, '--interactive=full']
        if os.path.isdir(os.path.join(home,"mysql-sandboxes",instance)):
            x_cmds = [
                      ("dba.killSandboxInstance(" + instance + ");\n", "successfully killed."),
                      ("dba.deleteSandboxInstance(" + instance + ");\n", "successfully deleted."),
                      ]
            results = exec_xshell_commands(init_command, x_cmds)
        elif os.path.isdir(os.path.join(cluster_Path,instance)):
            x_cmds = [
                      ("dba.killSandboxInstance(" + instance + ", { sandboxDir: \"" + cluster_Path + "\"});\n", "successfully killed."),
                      ("dba.deleteSandboxInstance(" + instance + ", { sandboxDir: \"" + cluster_Path + "\"});\n", "successfully deleted."),
                     ]
            results = exec_xshell_commands(init_command, x_cmds)
    except Exception, e:
        # kill instance failed
        print("kill instance"+instance+"Failed, "+e)
    return results


@timeout(240)
def exec_xshell_commands(init_cmdLine, commandList):
    RESULTS = "PASS"
    commandbefore = ""
    if "--sql"  in init_cmdLine:
        expectbefore = "mysql-sql>"
    elif "--sqlc"  in init_cmdLine:
        expectbefore = "mysql-sql>"
    elif "--py" in init_cmdLine:
        expectbefore = "mysql-py>"
    elif "--js" in init_cmdLine:
        expectbefore = "mysql-js>"
    else:
        expectbefore = "mysql-js>"
    p = subprocess.Popen(init_cmdLine, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    for command, lookup in commandList:
        # p.stdin.write(bytearray(command + "\n", 'ascii'))
        p.stdin.write(bytearray(command , 'ascii'))
        p.stdin.flush()
        # stdin,stdout = p.communicate()
        #found = read_til_getShell(p, p.stdout, lookup)
        found = read_til_getShell(p, p.stdout, expectbefore)
        if found.find(expectbefore, 0, len(found)) == -1:
            stdin,stdout = p.communicate()
            # return "FAIL \n\r"+stdin.decode("ascii") +stdout.decode("ascii")
            RESULTS="FAILED"
            return "FAIL: " + stdin.decode("ascii") + stdout.decode("ascii")
            break
        expectbefore = lookup
        commandbefore =command
    # p.stdin.write(bytearray(commandbefore, 'ascii'))
    p.stdin.write(bytearray('', 'ascii'))
    p.stdin.flush()
    #p.stdout.reset()
    stdin,stdout = p.communicate()
    found = stdout.find(bytearray(expectbefore,"ascii"), 0, len(stdout))
    if found == -1 and commandList.__len__() != 0 :
            found = stdin.find(bytearray(expectbefore,"ascii"), 0, len(stdin))
            if found == -1 :
                return "FAIL:  " + stdin.decode("ascii") + stdout.decode("ascii")
            else :
                return "PASS"
    else:
        return "PASS"


############   Retrieve variables from configuration file    ##########################
class LOCALHOST:
    user =""
    password = ""
    host = ""
    xprotocol_port = ""
    port =""
class REMOTEHOST:
    user = ""
    password =""
    host = ""
    xprotocol_port = ""
    port = ""

if 'CONFIG_PATH' in os.environ and 'MYSQLX_PATH' in os.environ and os.path.isfile(os.environ['CONFIG_PATH']) and os.path.isfile(os.environ['MYSQLX_PATH']):
    # **** JENKINS EXECUTION ****
    config_path = os.environ['CONFIG_PATH']
    config=json.load(open(config_path))
    MYSQL_SHELL = os.environ['MYSQLX_PATH']
    Exec_files_location = os.environ['AUX_FILES_PATH']
    cluster_Path = os.environ['CLUSTER_PATH']
    XSHELL_QA_TEST_ROOT = os.environ['XSHELL_QA_TEST_ROOT']
    XMLReportFilePath = XSHELL_QA_TEST_ROOT+"/adminapi_qa_test.xml"
else:
    # **** LOCAL EXECUTION ****
    config=json.load(open('config_local.json'))
    MYSQL_SHELL = str(config["general"]["xshell_path"])
    Exec_files_location = str(config["general"]["aux_files_path"])
    cluster_Path = str(config["general"]["cluster_path"])
    XMLReportFilePath = "adminapi_qa_test.xml"

#########################################################################

LOCALHOST.user = str(config["local"]["user"])
LOCALHOST.password = str(config["local"]["password"])
LOCALHOST.host = str(config["local"]["host"])
LOCALHOST.xprotocol_port = str(config["local"]["xprotocol_port"])
LOCALHOST.port = str(config["local"]["port"])

REMOTEHOST.user = str(config["remote"]["user"])
REMOTEHOST.password = str(config["remote"]["password"])
REMOTEHOST.host = str(config["remote"]["host"])
REMOTEHOST.xprotocol_port = str(config["remote"]["xprotocol_port"])
REMOTEHOST.port = str(config["remote"]["port"])



class globalvar:
    last_found=""
    last_search=""

###########################################################################################

class XShell_TestCases(unittest.TestCase):


  def test_MYS_690_reconfigure_when_removing_instance(self):
      '''MYS-690 [MYAA] reconfigure_when_removing_instance'''
      ################################ deploySandboxInstance 3312  #####################################################
      instance1 = "3312"
      kill_process(instance1)
      kill_process("3315")
      kill_process("3316")
      kill_process("3317")
      init_command = [MYSQL_SHELL, '--interactive=full', '--passwords-from-stdin']
      x_cmds = [("dba.deploySandboxInstance(" + instance1 + ", { sandboxDir: \"" + cluster_Path + "\"});\n",
                 'Please enter a MySQL root password for the new instance:'),
                (LOCALHOST.password + '\n',
                 "Instance localhost:" + instance1 + " successfully deployed and started."),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find(bytearray("FAIL", "ascii"), 0, len(results)) > -1:
          self.assertEqual(results, 'PASS')
      ################################ deploySandboxInstance 3313  #####################################################
      instance2 = "3313"
      kill_process(instance2)
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--passwords-from-stdin']
      x_cmds = [("dba.deploySandboxInstance(" + instance2 + ", { sandboxDir: \"" + cluster_Path + "\"});\n",
                 'Please enter a MySQL root password for the new instance:'),
                (LOCALHOST.password + '\n',
                 "Instance localhost:" + instance2 + " successfully deployed and started."),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find(bytearray("FAIL", "ascii"), 0, len(results)) > -1:
          self.assertEqual(results, 'PASS')
      ################################# deploySandboxInstance 3314  ###################################################
      instance3 = "3314"
      kill_process(instance3)
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--passwords-from-stdin']
      x_cmds = [("dba.deploySandboxInstance(" + instance3 + ", { sandboxDir: \"" + cluster_Path + "\"});\n",
                 'Please enter a MySQL root password for the new instance:'),
                (LOCALHOST.password + '\n',
                 "Instance localhost:" + instance3 + " successfully deployed and started."),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find(bytearray("FAIL", "ascii"), 0, len(results)) > -1:
          self.assertEqual(results, 'PASS')
      #################################### createCluster  #################################################
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + instance1, '--classic']
      x_cmds = [("dba.createCluster(\"devCluster\", {\"clusterAdminType\": \"local\"});\n", "<Cluster:devCluster>"),
                ("cluster = dba.getCluster('devCluster');\n", "<Cluster:devCluster>"),
                ("cluster.addInstance( \"{0}:{1}@{2}:3313?memberSsl=Trues\");\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                        LOCALHOST.host),
                 "was successfully added to the cluster"),
                ("cluster.addInstance( \"{0}:{1}@{2}:3314?memberSsl=true\");\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                        LOCALHOST.host),
                 "was successfully added to the cluster")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find(bytearray("FAIL", "ascii"), 0, len(results)) > -1:
          self.assertEqual(results, 'PASS')
      ########################## STOP INSTANCE

      instance="3312"
      results = ''
      init_command =  [MYSQL_SHELL, '--interactive=full', '--passwords-from-stdin']
      x_cmds = [("dba.stopSandboxInstance(" + instance1 + ", { sandboxDir: \"" + cluster_Path + "\"});\n",'successfully stopped.'),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

      #kill_process(instance1,"-STOP")
      #################################### stop an Instance 3312  #################################################
      results = ''
      # findString=\
      # "{"+os.linesep+\
      # "    \"clusterName\": \"devCluster\", "+os.linesep+\
      # "    \"defaultReplicaSet\": {"+os.linesep+\
      # "        \"name\": \"default\", "+os.linesep+\
      # "        \"status\": \"Cluster is NOT tolerant to any failures.\", "+os.linesep+\
      # "        \"topology\": {"+os.linesep+\
      # "            \"localhost:3313\": {"+os.linesep+\
      # "                \"address\": \"localhost:3313\", "+os.linesep+\
      # "                \"leaves\": {"+os.linesep+\
      findString= \
          "{"+os.linesep+\
          "     \"clusterName\": \"devCluster\","+os.linesep+\
          "     \"defaultReplicaSet\": {"+os.linesep+\
          "         \"name\": \"default\","+os.linesep
                  # "status": "Cluster is NOT tolerant to any failures.",
                  # "topology": {
                  #     "localhost:3313": {
                  #         "address": "localhost:3313",
                  #         "leaves": {
                  #             "localhost:3312": {
                  #                 "address": "localhost:3312",
                  #                 "leaves": {
                  #
                  #                 },
                  #                 "mode": "R/O",
                  #                 "role": "HA",
                  #                 "status": "OFFLINE"
                  #             },
                  #
                  #             "\"localhost:3312\": {"+os.linesep+ "\"address\": \"localhost:3312\", "+os.linesep
      # "                         \"address\": \"localhost:3312\", "+os.linesep+\
      # "                         \"leaves\": {"+os.linesep+\
      # " "+os.linesep+\
      # "                         }, "+os.linesep+\
      # "                         \"mode\": \"R/O\", "+os.linesep+\
      # "                         \"role\": \"HA\", "+os.linesep+\
      # "                         \"status\": \"OFFLINE\""+os.linesep
      # "                     }, "+os.linesep+\
      # "                     \"localhost:3314\": {"+os.linesep+\
      # "                         \"address\": \"localhost:3314\", "+os.linesep+\
      # "                         \"leaves\": {"+os.linesep
      # " "+os.linesep+\
      # "                         }, "+os.linesep+\
      # "                         \"mode\": \"R/O\", "+os.linesep+\
      # "                         \"role\": \"HA\", "+os.linesep+\
      # "                         \"status\": \"ONLINE\""+os.linesep+\
      # "                     }"+os.linesep+\
      # "                 }, "+os.linesep+\
      # "                 \"mode\": \"R/W\", "+os.linesep+\
      # "                 \"role\": \"HA\", "+os.linesep+\
      # "                 \"status\": \"ONLINE\""+os.linesep+\
      # "             }"+os.linesep+\
      # "         }"+os.linesep+\
      # "     }"+os.linesep+\
      # " }"+os.linesep
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + instance2, '--classic']
      x_cmds = [("cluster = dba.getCluster('devCluster');\n", "<Cluster:devCluster>"),
                ("cluster.status();\n", findString)
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find(bytearray("FAIL", "ascii"), 0, len(results)) > -1:
          self.assertEqual(results, 'PASS')
      ##########################

      #kill_process(instance2)
      #kill_process(instance3)


      self.assertEqual(results, 'PASS')




  # ----------------------------------------------------------------------
#
# if __name__ == '__main__':
#     unittest.main()

if __name__ == '__main__':
  unittest.main( testRunner=xmlrunner.XMLTestRunner(file(XMLReportFilePath,"w")))