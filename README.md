# detectproj.mzk.cz

[Detectproj](https://github.com/moravianlibrary/detectproj) is command line tool that detects projection of georeferenced maps (see [georeferencer.org](http://www.georeferencer.com/)).

The project detectproj.mzk.cz is FastCGI wrapper for Detectproj. It runs in Docker and caches computed projections in the SQLite database.

## API

Parameters for detectproj are passed through POST request in JSON format. The request has form:

```javascript
{
  "name" : "name of the map",
  "version" : "version of the map",
  "control_points" : [
    {
      "longitude" : number,
      "latitude"  : number,
      "pixel_x" : number,
      "pixel_y" : number
    },
    ...
  ]
}
```

###Possible responses

```javascript
{
  "status" : "Processed"
}
```

This response indicates that the process of projection detection has started and you must wait until it ends. The server cannot notice
you when the detection ends, so you must active ask it (in some periods) whether the computation has finished or not.

```javascript
{
  "status" : "Done",
  "projections" : [
    {
      "geojson" : {...}
    },
    {
      "geojson" : {...}
    },
    ...
  ]
}
```

This response indicates that the process of projection detection is done and also provides results of this detection. Element "projections"
contains list of found projections (the best three) and their geojson representations.

```javascript
{
  "status" : "Error",
  "message" : "Error description"
}
```

This response indicates that some error occurs during the processing of the message. For example: request has wrong format, some attributes are missing, json cannot be parsed...

```javascript
{
  "status" : "DetectprojError",
  "message" : "Error description"
}
```

This response indicates that some error occurs during the detection of the projection. Usually detectproj is not able to find projection
for specified points.

## Run

You can run the service simple by typing:

```
docker run -p 8080:80 -v /var/detectproj:/data moravianlibrary/detectproj.mzk.cz
```
Now the detectproj listen on http://localhost:8080/v1. Of course you can specify your own port (instead of 8080) and your own directory, which will be mapping to the /data in the docker container.
Be aware that the lighttpd server running inside the docker container must be able to write into this directory.

## Build

For building your own local docker image, you simple type

```
make

```

in the root directory of this github project. Now you are able to run your own build by command

```
docker run -p 8080:80 -v /var/detectproj:/data detectproj
```

## Setting detectproj as service using init script in debian based systems

You just create /etc/init.d/detectproj file that will contain following content

```bash
#!/bin/bash

### BEGIN INIT INFO
# Provides:          detectproj
# Required-Start:    docker
# Required-Stop:     docker
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Detectproj application
# Description:       Runs docker image.
### END INIT INFO

LOCK=/var/lock/detectproj
CONTAINER_NAME=detectproj.mzk.cz
IMAGE_NAME=moravianlibrary/detectproj.mzk.cz
PORT=9001

case "$1" in
  start)
    if [ -f $LOCK ]; then
      echo "Detectproj is running yet."
    else
      touch $LOCK
      echo "Starting detectproj.."
      docker run --name $CONTAINER_NAME -p $PORT:80 -v /var/detectproj/:/data $IMAGE_NAME &
      echo "[OK] Imagesearch is running."
    fi
    ;;
  stop)
    if [ -f $LOCK ]; then
      echo "Stopping detectproj.."
      rm $LOCK \
        && docker kill $CONTAINER_NAME \
        && docker rm $CONTAINER_NAME \
        && echo "[OK] Detectproj is stopped."
    else
      echo "Detectproj is not running."
    fi
    ;;
  restart)
    $0 stop
    $0 start
  ;;
  status)
    if [ -f $LOCK ]; then
      echo "Detectproj is running."
    else
      echo "Detectproj is not running."
    fi
  ;;
  update)
    docker pull $IMAGE_NAME
    $0 restart
  ;;
  *)
    echo "Usage: /etc/init.d/imagesearch {start|stop|restart|status|update}"
    exit 1
    ;;
esac

exit 0

```

After it run these commands

```
# chmod 755 /etc/init.d/detectproj
# update-rc.d detectproj defaults
```

Now the detectproj will be running automatically at boot time.

## Setting lighttpd server as proxy for detectproj

Usually more applications listening on port 80 run on one server. For this purpose it is necessary to set up
one application that will be accept all connections on the port 80 and it will forward them to appropriate application.
Assume that our detectproj listening on port 9001, than you can setup the lighttpd followingly:

1. Install lighttpd
2. Create file /etc/lighttpd/conf-available/20-detectproj.conf with content
```bash
$HTTP["host"] == "detectproj.mzk.cz" {
  proxy.server  = ( "" =>
                    (
                      ( "host" => "127.0.0.1",
                        "port" => 9001
                      )
                    )
                  )
}
$HTTP["host"] == "www.detectproj.mzk.cz" {
  proxy.server  = ( "" =>
                    (
                      ( "host" => "127.0.0.1",
                        "port" => 9001
                      )
                    )
                  )
}
```
3. Run
```
# lighttpd-enable-mod proxy
# lighttpd-enable-mod detectproj
# /etc/init.d/lighttpd restart
```
