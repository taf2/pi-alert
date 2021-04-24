require 'socket'
require 'timeout'

UDP_PORT = 1500

device_ip = ARGV[0]
if device_ip.nil?
  STDERR.puts "usage: ruby update.rb 192.168.x.x"
  exit 1
end

loop { 
  socket = UDPSocket.new
  socket.send("request:update", 0, device_ip, UDP_PORT)
  begin
    Timeout.timeout(0.4) do
      text, sender = socket.recvfrom(16)
      if text == 'Ack:Update'
        puts "ack update!"
        exit 0
      end
    end
  rescue Timeout::Error => e
  end
  socket.close
  sleep 0.1
  print(".")
}
