FROM alpine:latest

RUN cat /etc/alpine-release

RUN mkdir -p /scripts
COPY get-dependencies_linux.sh /scripts
RUN chmod +x /scripts/get-dependencies_linux.sh

RUN apk add --no-cache bash git build-base clang cmake ninja gdb p7zip gettext asciidoctor shadow \
 && /scripts/get-dependencies_linux.sh alpine build-dependencies

# Explicitly add /code as a "safe directory" (see: CVE-2022-24765)
RUN git config --global --add safe.directory /code

# Defines argument which can be passed during build time.
ARG USER=warzone2100
ARG UID=1000
# Create a user with given UID.
RUN useradd -d /home/warzone2100 -m -g root -G "$USER" -u "$UID" "$USER"
# Switch to ubuntu user by default.
USER warzone2100
# Check the current uid of the user.
RUN id

WORKDIR /code
CMD ["/bin/sh"]
