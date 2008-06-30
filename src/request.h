#ifndef _LIGHTTPD_REQUEST_H_
#define _LIGHTTPD_REQUEST_H_


typedef enum {
	HTTP_METHOD_UNSET = -1,
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_OPTIONS,
	HTTP_METHOD_PROPFIND,  /* WebDAV */
	HTTP_METHOD_MKCOL,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,
	HTTP_METHOD_COPY,
	HTTP_METHOD_MOVE,
	HTTP_METHOD_PROPPATCH,
	HTTP_METHOD_REPORT, /* DeltaV */
	HTTP_METHOD_CHECKOUT,
	HTTP_METHOD_CHECKIN,
	HTTP_METHOD_VERSION_CONTROL,
	HTTP_METHOD_UNCHECKOUT,
	HTTP_METHOD_MKACTIVITY,
	HTTP_METHOD_MERGE,
	HTTP_METHOD_LOCK,
	HTTP_METHOD_UNLOCK,
	HTTP_METHOD_LABEL,
	HTTP_METHOD_CONNECT
} http_method_t;

typedef enum {
	HTTP_VERSION_UNSET = -1,
	HTTP_VERSION_1_0,
	HTTP_VERSION_1_1
} http_version_t;

struct request;
typedef struct request request;

struct request_uri;
typedef struct request_uri request_uri;

struct physical;
typedef struct physical physical;

struct request_uri {
	GString *uri, *orig_uri;

	GString *scheme;
	GString *path;
	GString *query;
};

struct physical {
	GString *path;
	GString *basedir;

	GString *doc_root;
	GString *rel_path;

	GString *pathinfo;

	guint64 size;
};

struct request {
	http_method_t http_method;
	GString *http_method_str;
	http_version_t http_version;

	request_uri uri;

	GArray *headers;
	/* Parsed headers: */
	GString *host;
	goffset content_length;
};

#endif