#include "../inc/format_utils.h"
#include "json.h"
#include <sys/stat.h>
#include <stdlib.h>
#include "json.h"
#include "log.h"

#include <stdio.h>
#include <unistd.h>

#include <time.h>
#include "cyclonedx.h"
#include "spdx.h"

component_item component_list[CRC_LIST_LEN];

/* Check if a crc is found in the list (add it if not) */
bool add_CRC(uint32_t *list, uint32_t crc)
{
  for (int i = 0; i < CRC_LIST_LEN; i++)
  {
    if (list[i] == 0)
    {
      list[i] = crc;
      return false;
    }
    if (list[i] == crc)
      return true;
  }
  return false;
}

/* Check if a component is found in component_list (add it if not) */
bool add_component(match_data *match)
{
  /* Init component list */
  for (int i = 0; i < CRC_LIST_LEN; i++)
  {
    if (!strcmp(component_list[i].purl, match->purl))
    {
      log_debug("Component %s exist", match->purl);
      return true;
    }
    if (!*component_list[i].purl)
    {
      strcpy(component_list[i].license, match->license);
      strcpy(component_list[i].vendor, match->vendor);
      strcpy(component_list[i].component, match->component);
      strcpy(component_list[i].version, match->version);
      strcpy(component_list[i].latest_version, match->version);
      strcpy(component_list[i].purl, match->purl);
      log_debug("Component %s added", match->purl);
      return false;
    }
  }
  return false;
}

f_contents *scan_parse_read_file(char *filename)
{
  FILE *fp;
  struct stat filestatus;

  if (stat(filename, &filestatus) != 0)
  {
    return NULL;
  }
  f_contents *contents = calloc(1, sizeof(f_contents));
  contents->size = filestatus.st_size;
  contents->contents = (char *)malloc(filestatus.st_size);
  if (contents->contents == NULL)
  {
    free_f_contents(contents);
    return NULL;
  }

  fp = fopen(filename, "rt");
  if (fp == NULL)
  {
    fclose(fp);
    free_f_contents(contents);
    return NULL;
  }
  if (fread(contents->contents, contents->size, 1, fp) != 1)
  {
    fprintf(stderr, "Unable to read content of %s\n", filename);
    fclose(fp);
    free_f_contents(contents);
    return NULL;
  }
  fclose(fp);

  return contents;
}




int scan_parse_v2(char *filename)
{
  json_char *json;
  json_value *value;
  f_contents *contents = scan_parse_read_file(filename);
  if (!contents)
  {
    log_error("There was a problem reading file: %s", filename);
    return 1;
  }
  json = (json_char *)contents->contents;
  value = json_parse(json, contents->size);

  if (value == NULL)
  {
    log_error("Unable to parse data");
    free_f_contents(contents);
    return 1;
  }

  process_scan_result(value);

  json_value_free(value);
  free_f_contents(contents);
  return 0;
}

void free_f_contents(f_contents *c)
{
  if (c->contents)
    free(c->contents);
  free(c);
}
/* Returns a string with a hex representation of md5 */
char *md5_hex(uint8_t *md5)
{
  char *out = calloc(2 * MD5_LEN + 1, 1);
  for (int i = 0; i < MD5_LEN; i++)
    sprintf(out + strlen(out), "%02x", md5[i]);
  return out;
}

void process_scan_result(json_value *result)
{
  if (result == NULL)
  {
    return;
  }
  if (result->type != json_object)
  {
    return;
  }

  for (int i = 0; i < result->u.object.length; i++)
  {
    process_match(result->u.object.values[i]);
    log_trace("process %u/%u",i, result->u.object.length);
  }
}

void match_list_free(match_data_list *list)
{

  for (int i = 0; i < list->count; i++)
  {

    free(list->match_list[i]);
  }

  free(list->match_list);

  free(list);
}

void process_match(json_object_entry value)
{
  int array_length = value.value->u.array.length;
  //match_data_list *list = calloc(1, sizeof(match_data));
  //list->match_list = calloc(1, sizeof(match_data));
  //list->count = array_length;

  for (int i = 0; i < array_length; i++)
  {
    match_data *match = calloc(1, sizeof(match_data));
    json_object_entry *match_value = value.value->u.array.values[i]->u.object.values;
    int match_obj_len = value.value->u.array.values[i]->u.object.length;

   // list->match_list[i] = calloc(1, sizeof(match_data)); //we dont need a match list for the moment
    match_data * new_item = calloc(1, sizeof(match_data));
    for (int j = 0; j < match_obj_len; j++)
    {
      if (!strcmp(match_value[j].name, "id"))
      {
        if (strstr(match_value[j].value->u.string.ptr, "none"))
          break;
        strcpy(match->idtype, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "vendor"))
      {
        strcpy(new_item->vendor, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "component"))
      {
        strcpy(new_item->component, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "version"))
      {
        strcpy(new_item->version, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "latest"))
      {
        strcpy(new_item->latest_version, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "url"))
      {
        strcpy(new_item->url, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "release_date"))
      {
         strcpy(new_item->release_date, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "file"))
      {
         strcpy(new_item->filename, match_value[j].value->u.string.ptr);
      } 
      if (!strcmp(match_value[j].name, "purl"))
      {
        strcpy(new_item->purl, match_value[j].value->u.array.values[0]->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "licenses"))
      {
        if (match_value[j].value->u.array.length > 0)
        {
          strcpy(new_item->license, match_value[j].value->u.array.values[0]->u.object.values->value->u.string.ptr);
        }
      }   
      /*if (!strcmp(match_value[j].name, "lines"))
      {
         strcpy(match->lines, match_value[j].value->u.string.ptr);
      }*/
      /*if (!strcmp(match_value[j].name, "oss_lines"))
      {
         strcpy(match->oss_lines, match_value[j].value->u.string.ptr);
      }*/ //<---- was commented due to a bug on mac.
      if (!strcmp(match_value[j].name, "matched"))
      {
         strcpy(match->matched, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "size"))
      {
        strcpy(match->size, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "url_hash"))
      {
        strcpy(match->md5_comp, match_value[j].value->u.string.ptr);
       }
      if (!strcmp(match_value[j].name, "file"))
      {
        strcpy(match->filename, match_value[j].value->u.string.ptr);
      }
      if (!strcmp(match_value[j].name, "licenses"))
      {
        if (match_value[j].value->u.array.length > 0)
        {
           strcpy(match->license, match_value[j].value->u.array.values[0]->u.object.values->value->u.string.ptr);
         }
      }
    }
    add_component(new_item);
    free(match);
    free(new_item);
    //list->match_list[i] = new_item;
  }
 // match_list_free(list);
}

/* Output contents of component_list in the requested format */
void print_matches(FILE * output, char * format)
{
	bool cyclonedx = false;

  if (strstr(format,SCANNER_FORMAT_CYCLONEDX))
    cyclonedx = true;
    
  if (cyclonedx)
    cyclonedx_open(output);
  else
    spdx_open(output);

  for (int i = 0; i < CRC_LIST_LEN && *component_list[i].purl; i++)
	{
		if (i) 
      fprintf(output,"  ,\n");
	  
    if (cyclonedx) 
      print_json_match_cyclonedx(output, &component_list[i]);
  	else 
      print_json_match_spdx(output, &component_list[i]);
	}

   if (cyclonedx)
    cyclonedx_close(output);
  else
    spdx_close(output);

}


/* Returns the current date stamp */
char *datestamp(void)
{
	time_t timestamp;
	struct tm *times;
	time(&timestamp);
	times = localtime(&timestamp);
	char *stamp = malloc(MAX_ARGLN);
	strftime(stamp, MAX_ARGLN, "%FT%T%z", times);
	return stamp;
}