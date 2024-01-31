// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/spdx.c
 *
 * SPDX output handling
 *
 * Copyright (C) 2022, SCANOSS
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "spdx.h"




void spdx_open(FILE * output)
{
	fprintf(output,"{\n");
	fprintf(output,"  \"Document\": {\n");
	fprintf(output,"    \"specVersion\": \"SPDX-2.0\",\n");
	fprintf(output,"    \"creationInfo\": {\n");
	fprintf(output,"      \"creators\": [\n");
	fprintf(output,"        \"Tool: SCANOSS Inventory Engine\",\n");
	fprintf(output,"        \"Organization: http://scanoss.com\"\n");
	fprintf(output,"      ],\n");
	fprintf(output,"      \"comment\": \"This SPDX report has been automatically generated\",\n");
	fprintf(output,"      \"licenseListVersion\": \"1.19\",\n");

	fprintf(output,"      \"created\": \"");
	fprintf(output,"%s",datestamp());
	fprintf(output,"\"\n");

	fprintf(output,"    },\n");
	fprintf(output,"    \"spdxVersion\": \"SPDX-2.0\",\n");
	fprintf(output,"    \"dataLicense\": \"CC0-1.0\",\n");
	fprintf(output,"    \"id\": \"SPDXRef-DOCUMENT\",\n");
	fprintf(output,"    \"name\": \"SPDX-Tools-v2.0\",\n");
	fprintf(output,"    \"comment\": \"This document was automatically generated with SCANOSS.\",\n");
	fprintf(output,"    \"externalDocumentRefs\": [],\n");
	fprintf(output,"    \"Packages\": [\n");
}

void spdx_close(FILE * output)
{
	fprintf(output,"       ]\n");
	fprintf(output,"      }\n");
	fprintf(output,"}\n");
}

void print_json_match_spdx(FILE * output, component_item * comp_item)
{
	fprintf(output,"         {\n");
	fprintf(output,"          \"name\": \"%s\",\n", comp_item->component);

	if (strcmp(comp_item->version, comp_item->latest_version))
	{
		fprintf(output,"          \"versionInfo\": \"%s-%s\",\n", comp_item->version, comp_item->latest_version);
	}
	else
	{
		fprintf(output,"          \"versionInfo\": \"%s\",\n", comp_item->version);
	}

	fprintf(output,"          \"supplier\": \"%s\",\n", comp_item->vendor);
	fprintf(output,"          \"downloadLocation\": \"%s\",\n", comp_item->purl);
	fprintf(output,"          \"description\": \"Detected by SCANOSS Inventorying Engine.\",\n");
	fprintf(output,"          \"licenseConcluded\": \"\",\n");
	fprintf(output,"          \"licenseInfoFromFiles\": \"%s\"\n", comp_item->license);
	fprintf(output,"         }\n");

	fflush(stdout);
}
