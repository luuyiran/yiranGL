#ifndef __CUBE_MAP_H__
#define __CUBE_MAP_H__

unsigned int load_cube_map(const char *face[], int texture_should_flip);
unsigned int load_cube_map_hdr(const char *hdr, int texture_should_flip);
void cube_map_delete(unsigned int tex);

#endif
