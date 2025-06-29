
The ![Dockerfile](./Dockerfile) is built based on ubuntu 22.04 base image

### How to build

```
cd siege
docker build -t siege .

```

### How to use

```
# Start nginx container
docker run -d -p 8080:80 nginx:latest

# Get host ip
ifconfig [interface_name]

# Siege stress on nginx
docker run -it siege -c 100 -t 60S -q http://<ip>:8080

# Usage help
docker run -it siege --help

```
