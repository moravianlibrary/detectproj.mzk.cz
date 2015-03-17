DOCKER_NAME = detectproj

all: docker

docker:
	docker build -t $(DOCKER_NAME) .
