#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef _WIN32
# include <windows.h>
#endif

#include <curl/curl.h>

#include "memfile.h"
#include "from_url.h"

#define REQUEST_TIMEOUT (5)

CURLcode
memfile_from_url(const memfile_from_url_info info) {
  CURL* curl = curl_easy_init();
  if (!curl) return CURLE_FAILED_INIT;

  *info.body = memfopen();
  *info.header = memfopen();

  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt(curl, CURLOPT_URL, info.url);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, REQUEST_TIMEOUT);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, REQUEST_TIMEOUT);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, info.body_writer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, *info.body);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, info.header_writer);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, *info.header);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  const CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  return res;
}

static char*
get_http_header_alloc(const char* ptr, const char* key) {
  const size_t key_length = strlen(key);
  for (const char* tmp; *ptr && (tmp = strpbrk(ptr, "\r\n")); ptr = tmp + 1) {
    if (*(ptr + key_length) != ':' || strncasecmp(ptr, key, key_length))
      continue;

    // remove leading spaces
    const char* top = ptr + key_length + 1;
    while (*top && isspace(*top)) top++;
    if (!*top) break;

    const size_t len = tmp - top;
    char* val = malloc(len + 1);
    strncpy(val, top, len);
    val[len] = 0;
    return val;
  }
  return NULL;
}

GdkPixbuf*
pixbuf_from_url(const char* url, GError** error) {
  MEMFILE* mbody;
  MEMFILE* mhead;
  const CURLcode res = memfile_from_url((memfile_from_url_info){
    .url           = url,
    .body          = &mbody,
    .header        = &mhead,
    .body_writer   = memfwrite,
    .header_writer = memfwrite,
  });
  if (res == CURLE_FAILED_INIT) return NULL;

  char* head = memfstrdup(mhead);
  char* body = memfstrdup(mbody);
  unsigned long size = mbody->size;
  memfclose(mhead);
  memfclose(mbody);

  GdkPixbuf* pixbuf = NULL;

  if (res == CURLE_OK) {
    char* ctype = get_http_header_alloc(head, "Content-Type");
    char* csize = get_http_header_alloc(head, "Content-Length");

#ifdef _WIN32
    if (ctype && (!strcmp(ctype, "image/jpeg") || !strcmp(ctype, "image/gif"))) {
      char temp_path[MAX_PATH];
      char temp_filename[MAX_PATH];
      GetTempPath(sizeof(temp_path), temp_path);
      GetTempFileName(temp_path, "growl-for-linux-", 0, temp_filename);
      FILE* const fp = fopen(temp_filename, "wb");
      if (fp) {
        fwrite(body, size, 1, fp);
        fclose(fp);
      }
      pixbuf = gdk_pixbuf_new_from_file(temp_filename, NULL);
      DeleteFile(temp_filename);
    } else
#endif
    {
      GdkPixbufLoader* const loader =
        ctype ? gdk_pixbuf_loader_new_with_mime_type(ctype, error)
              : gdk_pixbuf_loader_new();
      if (csize) size = atol(csize);

      GError* _error = NULL;
      if (body && gdk_pixbuf_loader_write(loader, (const guchar*) body, size, &_error))
        pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
      else if (error)
        *error = _error;
      else if (_error)
        g_error_free(_error);

      if (loader) gdk_pixbuf_loader_close(loader, NULL);
    }
    free(ctype);
    free(csize);
  } else if (error) {
    *error = g_error_new_literal(G_FILE_ERROR, res, curl_easy_strerror(res));
  }

  free(head);
  free(body);

  return pixbuf;
}
