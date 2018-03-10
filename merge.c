#include "./record.c"
#include "./compare.h"


int merge(tax_rec* buf1, tax_rec* buf2, tax_rec* target_buf, int c1, int c2, int sort_by) {
	int comp, buf1_i = 0, buf2_i = 0, target_i = 0;
	while(buf1_i < c1 && buf2_i < c2) {
		switch(sort_by) {
			case 0:
				comp = cmp_by_id(&buf1[buf1_i], &buf2[buf2_i]);
				break;
			case 1:
				comp = cmp_by_fname(&buf1[buf1_i], &buf2[buf2_i]);
				break;
			case 2:
				comp = cmp_by_lname(&buf1[buf1_i], &buf2[buf2_i]);
				break;
			case 3:
				comp = cmp_by_income(&buf1[buf1_i], &buf2[buf2_i]);
				break;
			default:
				return -1;
		}
		if (comp <= 0) { 
			target_buf[target_i] = buf1[buf1_i];
			buf1_i++;
		} else {
			target_buf[target_i] = buf2[buf2_i];
			buf2_i++;
		}
		target_i++;
	}
	
	if (buf1_i < c1) {
		for (; buf1_i<c1; buf1_i++) {
			target_buf[target_i] = buf1[buf1_i];
			target_i++;
		}
	} else {
		for (; buf2_i<c2; buf2_i++) {
			target_buf[target_i] = buf2[buf2_i];
			target_i++;
		}
	}
	return target_i;

}