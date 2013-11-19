/*
The MIT License (MIT)

Copyright (c) 2013, Nick Gerakines

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <stdio.h>
#include <time.h>

typedef struct {
	ngx_flag_t enable;
	int range;
} ngx_http_timestamp_loc_conf_t;

static ngx_str_t ngx_http_timestamp_key = ngx_string("timestamp");

static ngx_int_t ngx_http_timestamp_handler(ngx_http_request_t* r);

static void* ngx_http_timestamp_create_loc_conf(ngx_conf_t* cf);
static char* ngx_http_timestamp_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child);
static ngx_int_t ngx_http_timestamp_init(ngx_conf_t* cf);

static ngx_command_t  ngx_http_timestamp_commands[] = {
	{
		ngx_string("timestamp"),
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_timestamp_loc_conf_t, enable),
		NULL
	},
	{
		ngx_string("timestamp_range"),
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
		ngx_conf_set_num_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_timestamp_loc_conf_t, range),
		NULL
	},
	ngx_null_command
};

static ngx_http_module_t ngx_http_timestamp_module_ctx = {
	NULL,
	ngx_http_timestamp_init,
	NULL,
	NULL,
	NULL,
	NULL,
	ngx_http_timestamp_create_loc_conf,
	ngx_http_timestamp_merge_loc_conf
};

ngx_module_t  ngx_http_timestamp_module = {
	NGX_MODULE_V1,
	&ngx_http_timestamp_module_ctx,
	ngx_http_timestamp_commands,
	NGX_HTTP_MODULE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_timestamp_handler(ngx_http_request_t* r) {
	ngx_uint_t i;
	ngx_http_timestamp_loc_conf_t*  alcf;
	alcf = ngx_http_get_module_loc_conf(r, ngx_http_timestamp_module);

	if (!alcf->enable) {
		return NGX_OK;
	}

	ngx_str_t args = r->args;
	ngx_uint_t j = 0, k = 0, l = 0;

	for (i = 0; i <= args.len; i++) {
		if ( ( i == args.len) || (args.data[i] == '&') ) {
			if (j > 1) {
				k = j;
				l = i;
			}

			j = 0;

		} else if ((j == 0) && (i < args.len - ngx_http_timestamp_key.len) ) {
			if ((ngx_strncmp(args.data + i, ngx_http_timestamp_key.data, ngx_http_timestamp_key.len) == 0) && (args.data[i + ngx_http_timestamp_key.len] == '=') ) {
				j = i + ngx_http_timestamp_key.len + 1;
				i = j - 1;

			} else {
				j = 1;
			}
		}
	}

	ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "timestamp: attempting to parse value of %d length from string that is %d", l - k, l);
	time_t timestamp_value;
	timestamp_value = ngx_atotm(args.data + k, l - k);

	if (timestamp_value == NGX_ERROR) {
		ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "timestamp: Could not parse timestamp from query string parameters");
		return NGX_HTTP_FORBIDDEN;
	}

	time_t now = time(0);
	ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "timestamp: the time is %d and given timestamp is %d", now, timestamp_value);
	int range = alcf->range;

	if (timestamp_value > now + range) {
		ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "timestamp: timestamp_value (%d) is above max range (%d; %d)", timestamp_value, now + range, range);
		return NGX_HTTP_FORBIDDEN;
	}

	if (timestamp_value < now - range) {
		ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "timestamp: timestamp_value (%d) is below min range (%d; %d)", timestamp_value, now - range, range);
		return NGX_HTTP_FORBIDDEN;
	}

	return NGX_OK;
}

static void* ngx_http_timestamp_create_loc_conf(ngx_conf_t* cf) {
	ngx_http_timestamp_loc_conf_t*  conf;
	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_timestamp_loc_conf_t));

	if (conf == NULL) {
		return NGX_CONF_ERROR;
	}

	conf->enable = NGX_CONF_UNSET;
	conf->range = NGX_CONF_UNSET;
	return conf;
}

static char* ngx_http_timestamp_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child) {
	ngx_http_timestamp_loc_conf_t* prev = parent;
	ngx_http_timestamp_loc_conf_t* conf = child;
	ngx_conf_merge_value(conf->enable, prev->enable, 0);
	ngx_conf_merge_value(conf->range, prev->range, 60 * 3);
	return NGX_CONF_OK;
}

static ngx_int_t ngx_http_timestamp_init(ngx_conf_t* cf) {
	ngx_http_handler_pt* h;
	ngx_http_core_main_conf_t* cmcf;
	cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
	h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);

	if (h == NULL) {
		return NGX_ERROR;
	}

	*h = ngx_http_timestamp_handler;
	return NGX_OK;
}
