FROM alpine:latest

RUN cat /etc/alpine-release

RUN mkdir -p /scripts
COPY get-dependencies_linux.sh /scripts
RUN chmod +x /scripts/get-dependencies_linux.sh

RUN apk add --no-cache bash git build-base clang cmake ninja gdb p7zip gettext asciidoctor \
 && /scripts/get-dependencies_linux.sh alpine build-dependencies

# Explicitly add /code as a "safe directory" (see: CVE-2022-24765)
RUN git config --global --add safe.directory /code

WORKDIR /code
CMD ["/bin/sh"]
