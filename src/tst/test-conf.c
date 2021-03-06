/**
 * Copyright (C) 2016, 2017  Sergey Morozov <sergey@morozov.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "conf.h"
#include "log.h"

static const char *test_conf_str = \
        "<General>\n"                                       \
        "    FsMountPoint             /foo/bar\n"           \
        "    LoggingFramework         simple\n"             \
        "    RemoteStoreProtocol      s3\n"                 \
        "    PathMax                  127\n"                \
        "</General>\n"                                      \
        "<Internal>\n"                                      \
        "    ScanfsIterTimeoutSec          100\n"           \
        "    MoveOutStartRate              0.8\n"           \
        "    MoveOutStopRate               0.7\n"           \
        "    PrimaryDownloadQueueMaxSize   111\n"           \
        "    SecondaryDownloadQueueMaxSize 222\n"           \
        "    PrimaryUploadQueueMaxSize     333\n"           \
        "    SecondaryUploadQueueMaxSize   444\n"           \
        "</Internal>\n"                                     \
        "<S3RemoteStore>\n"                                 \
        "    Hostname                 s3_hostname\n"        \
        "    Bucket                   s3.bucket\n"          \
        "    AccessKeyId              test_access_key_id\n" \
        "    SecretAccessKey          test_secret_key\n"    \
        "    TransferProtocol         https\n"              \
        "    OperationRetries         5\n"                  \
        "</S3RemoteStore>\n";

static int create_test_conf_file() {
        /* create configuration file */
        FILE *stream;

        stream = fopen("./test.conf", "w");
        if (!stream) {
                return -1;
        }

        size_t len_items = strlen(test_conf_str) + sizeof(char);
        if (fwrite(test_conf_str, sizeof(char), len_items, stream) != len_items) {
                return -1;
        }

        if (fclose(stream)) {
                return -1;
        }

        return 0;
}

int test_conf(char *err_msg) {
        if (create_test_conf_file() == -1) {
                strcpy(err_msg, "unable to create configuration file ./test.conf");
                return -1;
        }

        conf_t *conf = get_conf(); /* should be NULL before read_conf() call */
        if (conf != NULL) {
                strcpy(err_msg, "'get_conf()' should return NULL before 'read_conf()' was ever invoked");
                return -1;
        }

        /* validate read_conf() result */
        if (read_conf("./test.conf")) {
                strcpy(err_msg, "'read_conf()' function failed");
                return -1;
        }

        conf = get_conf();
        log_t *log = get_log();

        if (strcmp(conf->fs_mount_point, "/foo/bar") ||
            strcmp(conf->s3_default_hostname, "s3_hostname") ||
            strcmp(conf->s3_bucket, "s3.bucket") ||
            strcmp(conf->s3_access_key_id, "test_access_key_id") ||
            strcmp(conf->s3_secret_access_key, "test_secret_key") ||
            strcmp(conf->remote_store_protocol, "s3") ||
            strcmp(conf->transfer_protocol, "https") ||
            conf->scanfs_iter_tm_sec != 100 ||
            conf->move_out_start_rate != 0.8 ||
            conf->move_out_stop_rate != 0.7 ||
            conf->primary_download_queue_max_size != 111 ||
            conf->secondary_download_queue_max_size != 222 ||
            conf->primary_upload_queue_max_size != 333 ||
            conf->secondary_upload_queue_max_size != 444 ||
            conf->s3_operation_retries != 5 ||
            conf->path_max != (127 + 1) ||
            log->type != e_simple
        ) {
                strcpy(err_msg, "configuration contains incorrect values");
                return -1;
        }

        return 0;
}

