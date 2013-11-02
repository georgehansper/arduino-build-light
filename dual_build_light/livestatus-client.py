#!/usr/bin/python

import sys, getopt, time, socket
# For checking if file exists, and errno translation - os.strerror()
import os

BUFFER_SIZE = 4096

version = "$Id$"
# Events in the last t_recent will result in pulsing
t_recent = 120
# If no activity in t_quiescent change color to white
t_quiescent = 600
t_waittimeout = 15
verbose=0
ttyUSB_filename='/dev/ttyUSB0'
server_left=''
server_right=''
tcp_port_left=6557
tcp_port_right=6557
now=0

query_current_problems= \
"""GET services
Filter: acknowledged = 0
Filter: scheduled_downtime_depth = 0
Filter: host_scheduled_downtime_depth = 0
Stats: state = 0
Stats: state = 1
Stats: state = 2
Stats: state = 3
"""
query_current_problems_wait_for_event= \
"""GET services
Filter: acknowledged = 0
Filter: scheduled_downtime_depth = 0
Filter: host_scheduled_downtime_depth = 0
WaitTrigger: state
WaitTimeout: _waittimeout_
Stats: state = 0
Stats: state = 1
Stats: state = 2
Stats: state = 3
"""
query_recent_changes= \
"""GET services
Filter: acknowledged = 0
Filter: scheduled_downtime_depth = 0
Filter: host_scheduled_downtime_depth = 0
Filter: last_state_change > _when_
Stats: state = 0
Stats: state = 1
Stats: state = 2
Stats: state = 3
"""

def command_args(argv):
  global server_left, tcp_port_left, server_right, tcp_port_right, ttyUSB_filename, verbose
  global t_recent, t_quiescent, t_waittimeout
  try:
    opts, args = getopt.getopt(argv, 'l:r:d:Q:R:W:vVh', ['left=' ,'right=', 'dev=', 'recent=', 'quiescent=', 'wait','verbose', 'version','help'])
  except getopt.GetoptError:
    usage()
  try:
    for opt, arg in opts:
      #arg = arg.rstrip('%')
      if opt in ('-l', '--left'):
        servport = arg.split(':',2)
        server_left = servport[0]
	if len(servport)>1:
	  tcp_port_left = int(servport[1])
	print_v( "LEFT server=%s port=%d" % (server_left,tcp_port_left) )
      elif opt in ('-r', '--right'):
        servport = arg.split(':',2)
        server_right = servport[0]
	if len(servport)>1:
	  tcp_port_right = int(servport[1])
	print_v( "RIGHT server=%s port=%d" % (server_right,tcp_port_right) )
      elif opt in ('-d', '--dev'):
        ttyUSB_filename = arg
        if not os.path.exists(ttyUSB_filename):
          print "File does not exist: %s" % (ttyUSB_filename)
          sys.exit(1)
      elif opt in ('-R', '--recent'):
	t_recent = int(arg)
      elif opt in ('-Q', '--quiescent'):
	t_quiescent = int(arg)
      elif opt in ('-W', '--wait'):
	t_waittimeout = int(arg)
      elif opt in ('-v', '--verbose'):
        verbose = 1
      elif opt in ('-V', '--version'):
	print "livestatus-client.py for setting arduino dual_build_light"
        print "Version " + version
      elif opt in ('-h', '--help'):
        usage()
        sys.exit(0)
  except SystemExit:
    sys.exit(0)
  except:
    print "Invalid command line arg"
    sys.exit(1)
  if ( server_left == '' and server_right == '' ):
    print 'Must specify either -l or -r' + "\n"
    usage()
    sys.exit(1)

def usage():
	global tcp_port_left, t_recent, t_quiescent 
	print "Usage: " + sys.argv[0] + " [-d devfile] [-l left_server:port] [-r right_server:port]"
	print """
	-d, --dev     ... send LED commands to devfile (default is /dev/ttyUSB0)
	-l, --left    ... send livestatus queries to this server:port for controlling the left LED
	-r, --right   ... send livestatus queries to this server:port for controlling the right LED
	                  port is optional, and defaults to """ + str(tcp_port_left) + """
	-R, --recent  ... set recent time (default is """+ str(t_recent) + """s)
	-Q, --quiescent ... set quiescent time (default is """+ str(t_quiescent) + """s)
	-W, --wait    ... wait this many seconds before next poll of livestatus (alternates between servers) default is """ + str(t_waittimeout) + """

LEDs are set to PULSE if the current state is less than 'recent' seconds old
LEDs are set to WHITE if the current state is OK and more than 'quiescent' seconds old
"""

def print_v(msg):
	global verbose
	if verbose:
		print msg

def livestatus_query(server,tcp_port,q,t_when):
	global now
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
	  s.connect((server, tcp_port))
	  s.settimeout(3)
	  s.setblocking(1)
	except IOError, ioerr:
	    print "Error connecting to server: %s:%s - [%d] %s" % (server, tcp_port, ioerr.errno, os.strerror(ioerr.errno))
	    time.sleep(5)
	    return '0;0;0;1'
	  
	when = now -t_when
	if q.find('_when_') != -1:
	  q = q.replace('_when_',str(when))
	if q.find('_waittimeout_') != -1:
	  q = q.replace('_waittimeout_',str(t_waittimeout*1000))
	print_v(q)
	try:
	  s.send(q)
	  s.shutdown(socket.SHUT_WR)
	except IOError, ioerr:
	    print "Error sending to tcp connection %s:%s - [%d] %s" % (server, tcp_port, ioerr.errno, os.strerror(ioerr.errno))
	    time.sleep(5)
	    return '0;0;0;1'
	result=''
	while 1:
	  try:
	    data = s.recv(BUFFER_SIZE)
	    result += data
	  except IOError, ioerr:
	    print "Error reading tcp connection %s:%s - [%d] %s" % (server, tcp_port, ioerr.errno, os.strerror(ioerr.errno))
	    result = '0;0;0;1'
	    time.sleep(5)
	    data = ''
	  except FileNotFoundError:
	    print "File not found: %s:%s\n%s" % (server, tcp_port, '')
	    sys.exit(1)
	  #print "received data:", data
	  if len(data) == 0:
	    break
	s.close()
	print_v('------ Livestatus Result -----')
	print_v(result)
	print_v('------')
	return result

def set_leds(txt):
	global ttyUSB_filename
	print_v("Writing to %s:\n%s\n" % (ttyUSB_filename,txt))
	try:
		ttyUSB = open(ttyUSB_filename,'a')
		ttyUSB.write(txt + "\n")
		ttyUSB.close()
	except IOError, ioerr:
		print "Error opening file %s - [%d] %s" % (ttyUSB_filename, ioerr.errno, os.strerror(ioerr.errno))


def main():
	global verbose,now
	global t_recent,t_quiescent
	q_ndx=0
	ttyUSB_filename='out.dalek.txt'

	command_args(sys.argv[1:])

	while 1:
		set_leds('PERIOD=1000')
		now = int(time.time())
		print_v("Current time: " + str(int(now)))
		for (server,tcp_port,side) in [(server_left,tcp_port_left,'LEFT'),(server_right,tcp_port_right,'RIGHT')]:
		  if server == '':
		    continue
		  print_v("server=%s, side=%s" % (server,side))
		  current = []
		  recent = []
		  quiescent = []
		  current_txt = livestatus_query(server,tcp_port,query_current_problems_wait_for_event,0)
		  recent_txt = livestatus_query(server,tcp_port,query_recent_changes,t_recent)
		  current_txt = current_txt.rstrip()
		  current = current_txt.split(';')
		  recent_txt = recent_txt.rstrip()
		  recent = recent_txt.split(';')
		  if len(current) != 4 or len(recent) != 4:
		    time.sleep(5)
		    continue
		  if int(current[2])>0:
		     # Critical - Red
		     if int(recent[2])>0:
		       set_leds('%s=%s PULSE' % (side,'255,0,0,60000'))
		     else:
		       set_leds('%s=%s' % (side,'255,0,0,60000'))
		  elif int(current[3])>0:
		     # Unknown - Amber
		     if int(recent[3])>0:
		       set_leds('%s=%s PULSE' % (side,'255,80,0,60000'))
		     else:
		       set_leds('%s=%s' % (side,'255,80,0,60000'))
		  elif int(current[1])>0:
		     # Warning - Yellow
		     if int(recent[1])>0:
		       set_leds('%s=%s PULSE' % (side,'255,128,0,60000'))
		     else:
		       set_leds('%s=%s' % (side,'255,128,0,60000'))
		  elif int(current[0])>0:
		     # OK - Green
		     if int(recent[0])>0:
		       set_leds('%s=%s PULSE' % (side,'0,255,0,60000'))
		     else:
		       set_leds('%s=%s' % (side,'0,255,0,60000'))
		       quiescent_txt = livestatus_query(server,tcp_port,query_recent_changes,t_quiescent)
		       quiescent_txt = quiescent_txt.rstrip()
		       quiescent = quiescent_txt.split(';')
		       if int(quiescent[0])==0:
			  #  Nothing going on for a while - Steady white
			  set_leds('%s=%s STEADY' % (side,'255,200,200,60000'))
		  print_v  (" - ".join(current))
		  print_v  (" - ".join(recent))
		  print_v  (" - ".join(quiescent))

if __name__ == '__main__':
  main()

sys.exit(0)
