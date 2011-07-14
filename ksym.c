#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <kvm.h>
#include <nlist.h>
#include <limits.h>

char ksym_err[_POSIX2_LINE_MAX];
int get_ksym(char* sym, void* buf, size_t size) {
    kvm_t *kd;
    struct nlist nl[] = {
	{ NULL },
	{ NULL },
    };
    
    nl[0].n_name = sym;
    
    kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, ksym_err);
    if(kd == NULL) return -1;
    
    if(kvm_nlist(kd, nl) < 0) {
	strncpy(ksym_err, kvm_geterr(kd), sizeof(ksym_err));
	kvm_close(kd);
	return -1;
    }
    
    if(nl[0].n_value <= 0) {
	snprintf(ksym_err, sizeof(ksym_err), "Symbol %s not found.", sym);
	kvm_close(kd);
	return -1;
    }
    
    if(kvm_read(kd, nl[0].n_value, buf, size) < 0) {
	strncpy(ksym_err, kvm_geterr(kd), sizeof(ksym_err));
	kvm_close(kd);
	return -1;
    }

    if(kvm_close(kd) < 0) {
	strncpy(ksym_err, kvm_geterr(kd), sizeof(ksym_err));
	return -1;
    }
    
    return 0;
}
