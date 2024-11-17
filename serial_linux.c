#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

void set_flags(int serial_port, struct termios tty) {
  if (tcgetattr(serial_port, &tty) != 0) {
    printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    exit(1);
  }
  // Clear parity bit, disabling parity (most common)
  tty.c_cflag &= ~PARENB;
  // Clear stop field, only one stop bit used in
  // communication (most common)
  tty.c_cflag &= ~CSTOPB;
  // Clear all bits that set the data size
  tty.c_cflag &= ~CSIZE;
  // 8 bits per byte (most common)
  tty.c_cflag |= CS8;
  // Disable RTS/CTS hardware flow control (most common)
  tty.c_cflag &= ~CRTSCTS;
  // Turn on READ & ignore ctrl lines (CLOCAL = 1)
  tty.c_cflag |= CREAD | CLOCAL;

  tty.c_lflag &= ~ICANON;
  // Disable echo
  tty.c_lflag &= ~ECHO;
  // Disable erasure
  tty.c_lflag &= ~ECHOE;
  // Disable new-line echo
  tty.c_lflag &= ~ECHONL;
  // Disable interpretation of INTR, QUIT and SUSP
  tty.c_lflag &= ~ISIG;
  // Turn off s/w flow ctrl
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  // Disable any special handling of received bytes
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

  // Prevent special interpretation of output bytes (e.g.
  // newline chars)
  tty.c_oflag &= ~OPOST;
  // Prevent conversion of newline to carriage return/line feed
  tty.c_oflag &= ~ONLCR;
  // Wait for up to 1s (10 deciseconds), returning as soon
  // as any data is received.
  tty.c_cc[VTIME] = 50;

  tty.c_cc[VMIN] = 0;

  // Set in/out baud rate to be 9600
  cfsetispeed(&tty, B9600);
  cfsetospeed(&tty, B9600);

  // Save tty settings, also checking for error
  if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    exit(1);
  }
}

int main() {
  int serial_port = open("/dev/ttyUSB0", O_RDWR);
  struct termios tty;

  set_flags(serial_port, tty);

  char read_buf[256];
  memset(&read_buf, '\0', sizeof(read_buf));
  int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
  if (num_bytes < 0) {
    printf("Error reading: %s", strerror(errno));
    return 1;
  }
  printf("Read %i bytes. INIT message: %s", num_bytes, read_buf);

  unsigned char msg[] = {'H', 'e', 'l', 'l', 'o', '\n'};
  write(serial_port, msg, sizeof(msg));

  memset(&read_buf, '\0', sizeof(read_buf));
  num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
  if (num_bytes < 0) {
    printf("Error reading: %s", strerror(errno));
    return 1;
  }
  printf("Read %i bytes. Received message: %s", num_bytes, read_buf);

  close(serial_port);
  return 0;
};
