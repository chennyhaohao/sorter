#include <string.h>
#include "./record.c"

//Functions that compare 2 records based on different parameters
//returns >0 if rec1 greater than rec2, ==0 if the two are equal, and <0 if rec1 smaller than rec2

int cmp_by_id(const void * rec1, const void * rec2) {
	return ((tax_rec*)rec1)->id - ((tax_rec*)rec2)->id;
}

int cmp_by_fname(const void * rec1, const void * rec2) {
	return strncmp(((tax_rec*)rec1)->fname, ((tax_rec*)rec2)->fname, 25);
}

int cmp_by_lname(const void * rec1, const void * rec2) {
	return strncmp(((tax_rec*)rec1)->lname, ((tax_rec*)rec2)->lname, 35);
}

int cmp_by_income(const void * rec1, const void * rec2) {
	return ((tax_rec*)rec1)->income - ((tax_rec*)rec2)->income;
}