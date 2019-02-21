/*
 * AoE1 DRS extractor
 * https://github.com/vs49688/extract-drs

 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Zane van Iperen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <cstring>
#include <memory>

struct stdio_deleter { void operator()(FILE *f) noexcept { fclose(f); } };
using stdio_ptr = std::unique_ptr<FILE, stdio_deleter>;

#pragma pack(push, 1)
typedef struct drs_header
{
	char		notice[36]; /* "Copyright (c) 1997 Ensemble Studios." */
	uint32_t	version;
	char		tribe[16]; /* "1.00tribe". */
	uint32_t	directory_count;
	uint32_t	data_offset;
} drs_header_t;

typedef struct drs_dinfo
{
	uint8_t		flag;	/* Not sure what this is. */
	char		ext[3];
	uint32_t	offset;
	uint32_t	file_count;
} drs_dinfo_t;

typedef struct drs_dentry
{
	uint32_t	id;
	uint32_t	offset;
	uint32_t	size;
} drs_dentry_t;
#pragma pack(pop)

/* If this fails, errno will always be set. */
static int drs_write_file(FILE *f, drs_dinfo_t *info, drs_dentry_t *entry)
{
	if(fseek(f, entry->offset, SEEK_SET) < 0)
		return -1;

	char name[32];
	snprintf(name, sizeof(name), "%u.%c%c%c", entry->id, info->ext[2], info->ext[1], info->ext[0]);

	void *buf = malloc(entry->size);
	if(!buf)
		return -1;

	FILE *of = fopen(name, "wb");
	if(!of)
		goto fopen_fail;

	if(fread(buf, entry->size, 1, f) != 1)
		goto io_fail;

	if(fwrite(buf, entry->size, 1, of) != 1)
		goto io_fail;

	return 0;
io_fail:
	fclose(of);
fopen_fail:
	free(buf);
	return -1;
}

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		printf("Usage: %s <infile.drs>\n", argv[0]);
		return 2;
	}

	stdio_ptr f(fopen(argv[1], "rb"));
	if(!f)
	{
		fprintf(stderr, "Unable to open \"%s\": %s\n", argv[1], strerror(errno));
		return 1;
	}

	drs_header_t h;
	if(fread(&h, sizeof(h), 1, f.get()) != 1)
	{
		perror("fread");
		return 1;
	}

	if(h.version != 26)
	{
		fprintf(stderr, "Unknown file version, got %d, expected 26...\n", h.version);
		return 1;
	}

	drs_dinfo_t *dirinfo = reinterpret_cast<drs_dinfo_t*>(alloca(sizeof(drs_dinfo_t) * h.directory_count));
	fread(dirinfo, sizeof(drs_dinfo_t) * h.directory_count, 1, f.get());

	uint32_t max_files = 0;
	{
		for(size_t i = 0; i < h.directory_count; ++i)
			max_files = std::max(max_files, dirinfo[i].file_count);
	}

	drs_dentry_t *dir = reinterpret_cast<drs_dentry_t*>(malloc(sizeof(drs_dentry_t) * max_files));
	if(!dir)
	{
		perror("malloc");
		return 1;
	}

	for(size_t d = 0; d < h.directory_count; ++d)
	{
		fseek(f.get(), dirinfo[d].offset, SEEK_SET);
		fread(dir, sizeof(drs_dentry_t) * dirinfo[d].file_count, 1, f.get());

		for(size_t i = 0; i < dirinfo[d].file_count; ++i)
		{
			if(drs_write_file(f.get(), &dirinfo[d], &dir[i]) < 0)
				fprintf(stderr, "Error extracting file %u: %s\n", dir[i].id, strerror(errno));
		}
	}

	free(dir);
	return 0;
}