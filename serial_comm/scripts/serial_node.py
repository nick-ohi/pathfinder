#!/usr/bin/env python
import rospy
import serial
import time
import serial_comm_class

class Serial_Port:
	# Constructor opens and flushes serial port
	#~~~ confirm values from parameter server override defaults
	def __init__(self):
		port_path = rospy.get_param('port_path', '/dev/ttyS0')
		self.ser = serial.Serial(port_path,115200,serial.EIGHTBITS,serial.PARITY_NONE,serial.STOPBITS_ONE,0,False,False,None,False,None)
		self.ser.flushInput()
		self.ser.flushOutput()
		node_type = rospy.get_param('node_type', 'mis_det_serial_node')
		self.comm = serial_comm_class.Serial_Comm(node_type)
		self.rate_Hz = rospy.get_param('rate_Hz', 10)
		rospy.init_node(node_type)
		if (self.comm.reader):
			self.pub = rospy.Publisher(self.comm.topic_out, self.comm.pkt_in.msg_class) 
		if (self.comm.writer):
			self.sub = rospy.Subscriber(self.comm.topic_in, self.comm.pkt_out.msg_class, self.comm.pkt_out.pack_msg2buffer)
		self.execution_rate = rospy.Rate(self.rate_Hz*2)


	def write_buff(self, out_pkt):
		i = 0
		out_pkt.packet.fields.checksum = out_pkt.compute_checksum()
		size = len(out_pkt.packet.buffer)
		while i < size:
			n=chr(out_pkt.packet.buffer[i])
			self.ser.write(n)
			i=i+1
		out_pkt.increment_counter()
			
	
	# run() implements the main loop for the node
	def run(self):

		prior_time = time.clock()
		
		# read new packet from serial port
		while not rospy.is_shutdown():
			if (self.comm.reader):
				#rospy.loginfo("Reader = True")
				m=self.readChar()
				#rospy.loginfo("%s %d",m,self.comm.pkt_in.packet.fields.H1)
				if (len(m) != 0) and (ord(m)==self.comm.pkt_in.packet.fields.H1):
					#rospy.loginfo("Read H1")
					m=self.readChar()
					if (len(m) != 0) and (ord(m)==self.comm.pkt_in.packet.fields.H2):
						#rospy.loginfo("Read H2")
						m=self.readChar()
						if (len(m) != 0) and (ord(m)==self.comm.pkt_in.packet.fields.H3):
							#rospy.loginfo("Read H3")
							i=3
							size = len(self.comm.pkt_in.packet.buffer)
							while i < size:
								#rospy.loginfo("Reading %d",i)
								m=self.readChar()
								if len(m) != 0:
									#rospy.loginfo("Char read %s",m)
									n=ord(m)
								self.comm.pkt_in.packet.buffer[i] = n
								i=i+1

							#rospy.loginfo("Checksum = %d, valid  = %d",self.comm.pkt_in.packet.fields.checksum, self.comm.pkt_in.valid_checksum())
							#rospy.loginfo("%d %d",self.comm.pkt_in.packet.buffer[3],self.comm.pkt_in.packet.buffer[4])
							#rospy.loginfo(self.comm.pkt_in.packet.buffer[3]*256+self.comm.pkt_in.packet.buffer[4])
							#rospy.loginfo("Calculated Checksum = %d",self.comm.pkt_in.compute_checksum().value)
							# transfer data from serial packet to ROS msg and publish
							if (self.comm.pkt_in.valid_checksum()):
								self.comm.pkt_in.unpack_buffer2msg()
								self.pub.publish(self.comm.msg_out)

			# check for serial output time and write to the port
			if (self.comm.writer):
				period = 1/float(self.rate_Hz)
				current_time = time.clock()	
#~~~				rospy.loginfo("writer=True: current=%g, prior=%g, rate=%d, period=%g", current_time, prior_time, self.comm.pkt_out.rate_Hz, period)
				if (current_time - prior_time) >= period:  
					# write to the port with the latest data stored in the ouput packet buffer
#~~~					rospy.loginfo("Writing %d bytes", len(self.comm.pkt_out.packet.buffer))
					self.write_buff(self.comm.pkt_out)
					prior_time = current_time
			
			#~~~running the read loop at twice the port rate--could cause lag, but minimizes cpu usage
			self.execution_rate.sleep()
		
		# close the port on shutdown
		self.ser.close()
	
	def readChar(self):
		initial_time = time.clock()
		timeout = False
		timeout_time = 0.1
		m = self.ser.read(1)
		while (len(m) == 0) and (timeout == False):
			m = self.ser.read(1)
			current_time = time.clock()
			if (current_time - initial_time) >= timeout_time:
				timeout = True
			else:
				timeout = False
		return m


# launch run() for main node loop
if __name__ == '__main__':
	try:
		port = Serial_Port()
		port.run()
	except rospy.ROSInterruptException:
		pass
					
