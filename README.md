
# nginx\_timestamp

To build:

    (from the nginx source dir)
    $ ./configure --add-module=path/to/nginx_timestamp/
    $ make

To run:

    (from project source dir)
    $ cat > EOF
    worker_processes  1;
    error_log  logs/error.log info;
    pid        logs/nginx.pid;
    events {
        worker_connections  32;
    }
    http {
        default_type  text/html;
        access_log  logs/access.log;
        keepalive_timeout  65;
        server {
            listen       8080;
            server_name  localhost;
            location / {
    			timestamp on;
                root   www;
                index  index.html;
            }
        }
    }
    EOF
    $ path/to/nginx-1.5.7/objs/nginx -c `pwd`/nginx.conf -p ./

To test:

    $ curl "http://localhost:8080/?timestamp=1384892650"
