#thin start -p 4567 -R httpd.rb -a 192.168.2.120  -D -V 
# note as of 2020 thin can't handle transfer encoding chunked
#ruby httpd.rb -s puma -o 192.168.2.120 -p 4567
#rackup -o 192.168.2.120 -p 4567 -E none
FLASK_APP=httpd.py flask run --reload --host 192.168.2.120 -p 4567
