#include "inttypes.h"
#include "MDAC_conf.h"
#include "net_interface.h"

//Web site structure
#define WWW_MAIN 0
#define WWW_IP_MAC 1
#define WWW_FAVICON 2
#define WWW_CONN 3
//extern uint8_t www_pages[];

extern const char* www_pages[4];

void http200ok(void);
void print_favico(void);
void moved_perm(const char *);

void print_mainpage(void);
int8_t handle_mainpage_action(char *);

void print_ip_mac_page(void);
void print_connections(void);
	
void edit_ip_mac( char*, struct net_interface_state*);
void www_ans(struct net_interface_state*);
