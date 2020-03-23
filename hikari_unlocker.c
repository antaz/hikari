#include <pwd.h>
#include <security/pam_appl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>

static char *input_buffer = NULL;

#define INPUT_BUFFER_SIZE 1024

static int
conversation_handler(int num_msg,
    const struct pam_message **msg,
    struct pam_response **resp,
    void *data)
{
  struct pam_response *pam_reply = calloc(num_msg, sizeof(struct pam_response));

  if (pam_reply == NULL) {
    return PAM_ABORT;
  }
  *resp = pam_reply;
  for (int i = 0; i < num_msg; ++i) {
    switch (msg[i]->msg_style) {
      case PAM_PROMPT_ECHO_OFF:
      case PAM_PROMPT_ECHO_ON:
        pam_reply[i].resp = strdup(input_buffer);
        if (pam_reply[i].resp == NULL) {
          return PAM_ABORT;
        }
        break;

      case PAM_ERROR_MSG:
      case PAM_TEXT_INFO:
        break;
    }
  }
  return PAM_SUCCESS;
}

bool
check_password(const char *username)
{
  const struct pam_conv conv = {
    .conv = conversation_handler,
    .appdata_ptr = NULL,
  };

  bool success = false;
  pam_handle_t *auth_handle = NULL;
  if (pam_start("hikari-unlocker", username, &conv, &auth_handle) !=
      PAM_SUCCESS) {
    return false;
  }

  read(0, input_buffer, INPUT_BUFFER_SIZE - 1);
  int pam_status = pam_authenticate(auth_handle, 0);
  memset(input_buffer, 0, INPUT_BUFFER_SIZE);
  success = pam_status == PAM_SUCCESS;
  write(1, &success, sizeof(bool));

  pam_end(auth_handle, pam_status);

  return success;
}

int
main(int argc, char **argv)
{
  char input;
  bool success = false;
  struct passwd *passwd = getpwuid(getuid());

  input_buffer = malloc(INPUT_BUFFER_SIZE);
  memset(input_buffer, 0, INPUT_BUFFER_SIZE);
  mlock(input_buffer, INPUT_BUFFER_SIZE);

  while (!success) {
    success = check_password(passwd->pw_name);
  }

  munlock(input_buffer, INPUT_BUFFER_SIZE);
  free(input_buffer);

  return 0;
}
