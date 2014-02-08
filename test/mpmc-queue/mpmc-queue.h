#include <stdint.h>
#include <unrelacy.h>
#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <common.h>
#include <specannotation.h>
#include <modeltypes.h>
#include <stdatomic.h>
#include <model_memory.h>
#include <stdatomic.h>
#include <unrelacy.h>
#include <common.h>
template < typename t_element , size_t t_size >
struct mpmc_boundq_1_alt {
private : t_element m_array [ t_size ] ;
atomic < unsigned int > m_rdwr ;
atomic < unsigned int > m_read ;
atomic < unsigned int > m_written ;
public : mpmc_boundq_1_alt ( ) {
__sequential_init();
m_rdwr = 0 ;
m_read = 0 ;
m_written = 0 ;
}
/* All other user-defined structs */
typedef struct elem {
t_element * pos ;
bool written ;
thread_id_t tid ;
thread_id_t fetch_tid ;
call_id_t id ;
}
elem ;

static spec_list * list;
static id_tag_t * tag;
/* All other user-defined functions */
inline static elem * new_elem ( t_element * pos , call_id_t id , thread_id_t tid ) {
elem * e = ( elem * ) MODEL_MALLOC ( sizeof ( elem ) ) ;
e -> pos = pos ;
e -> written = false ;
e -> id = id ;
e -> tid = tid ;
e -> fetch_tid = - 1 ;
}

inline static elem * get_elem_by_pos ( t_element * pos ) {
for ( int i = 0 ; i < size ( list ) ; i ++ ) {
elem * e = ( elem * ) elem_at_index ( list , i ) ; if ( e -> pos == pos ) {
return e ; }
}
return NULL ;
}

inline static void show_list ( ) {
for ( int i = 0 ; i < size ( list ) ; i ++ ) {
elem * e = ( elem * ) elem_at_index ( list , i ) ; }
}

inline static elem * get_elem_by_tid ( thread_id_t tid ) {
for ( int i = 0 ; i < size ( list ) ; i ++ ) {
elem * e = ( elem * ) elem_at_index ( list , i ) ; if ( e -> tid == tid ) {
return e ; }
}
return NULL ;
}

inline static elem * get_elem_by_fetch_tid ( thread_id_t fetch_tid ) {
for ( int i = 0 ; i < size ( list ) ; i ++ ) {
elem * e = ( elem * ) elem_at_index ( list , i ) ; if ( e -> fetch_tid == fetch_tid ) {
return e ; }
}
return NULL ;
}

inline static int elem_idx_by_pos ( t_element * pos ) {
for ( int i = 0 ; i < size ( list ) ; i ++ ) {
elem * existing = ( elem * ) elem_at_index ( list , i ) ; if ( pos == existing -> pos ) {
return i ; }
}
return - 1 ;
}

inline static int elem_idx_by_tid ( thread_id_t tid ) {
for ( int i = 0 ; i < size ( list ) ; i ++ ) {
elem * existing = ( elem * ) elem_at_index ( list , i ) ; if ( tid == existing -> tid ) {
return i ; }
}
return - 1 ;
}

inline static int elem_idx_by_fetch_tid ( thread_id_t fetch_tid ) {
for ( int i = 0 ; i < size ( list ) ; i ++ ) {
elem * existing = ( elem * ) elem_at_index ( list , i ) ; if ( fetch_tid == existing -> fetch_tid ) {
return i ; }
}
return - 1 ;
}

inline static int elem_num ( t_element * pos ) {
int cnt = 0 ;
for ( int i = 0 ; i < size ( list ) ; i ++ ) {
elem * existing = ( elem * ) elem_at_index ( list , i ) ; if ( pos == existing -> pos ) {
cnt ++ ; }
}
return cnt ;
}

inline static call_id_t prepare_id ( ) {
call_id_t res = get_and_inc ( tag ) ;
return res ;
}

inline static bool prepare_check ( t_element * pos , thread_id_t tid ) {
show_list ( ) ;
elem * e = get_elem_by_pos ( pos ) ;
return NULL == e ;
}

inline static void prepare ( call_id_t id , t_element * pos , thread_id_t tid ) {
elem * e = new_elem ( pos , id , tid ) ;
push_back ( list , e ) ;
}

inline static call_id_t publish_id ( thread_id_t tid ) {
elem * e = get_elem_by_tid ( tid ) ;
if ( NULL == e ) return DEFAULT_CALL_ID ;
return e -> id ;
}

inline static bool publish_check ( thread_id_t tid ) {
show_list ( ) ;
elem * e = get_elem_by_tid ( tid ) ;
if ( NULL == e ) return false ;
if ( elem_num ( e -> pos ) > 1 ) return false ;
return ! e -> written ;
}

inline static void publish ( thread_id_t tid ) {
elem * e = get_elem_by_tid ( tid ) ;
e -> written = true ;
}

inline static call_id_t fetch_id ( t_element * pos ) {
elem * e = get_elem_by_pos ( pos ) ;
if ( NULL == e ) return DEFAULT_CALL_ID ;
return e -> id ;
}

inline static bool fetch_check ( t_element * pos ) {
show_list ( ) ;
if ( pos == NULL ) return true ;
elem * e = get_elem_by_pos ( pos ) ;
if ( e == NULL ) return false ;
if ( elem_num ( e -> pos ) > 1 ) return false ;
return true ;
}

inline static void fetch ( t_element * pos , thread_id_t tid ) {
if ( pos == NULL ) return ;
elem * e = ( elem * ) get_elem_by_pos ( pos ) ;
e -> fetch_tid = tid ;
}

inline static bool consume_check ( thread_id_t tid ) {
show_list ( ) ;
elem * e = get_elem_by_fetch_tid ( tid ) ;
if ( NULL == e ) return false ;
if ( elem_num ( e -> pos ) > 1 ) return false ;
return e -> written ;
}

inline static call_id_t consume_id ( thread_id_t tid ) {
elem * e = get_elem_by_fetch_tid ( tid ) ;
if ( NULL == e ) return DEFAULT_CALL_ID ;
return e -> id ;
}

inline static void consume ( thread_id_t tid ) {
int idx = elem_idx_by_fetch_tid ( tid ) ;
if ( idx == - 1 ) return ;
remove_at_index ( list , idx ) ;
}

/* ID function of interface: Publish */
inline static call_id_t Publish_id(void *info, thread_id_t __TID__) {
call_id_t __ID__ = publish_id ( __TID__ );
return __ID__;
}
/* End of ID function: Publish */

/* Check action function of interface: Publish */
inline static bool Publish_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
bool check_passed;
check_passed = publish_check ( __TID__ );
if (!check_passed) return false;
publish ( __TID__ ) ;
return true;
}
/* End of check action function: Publish */

/* Definition of interface info struct: Fetch */
typedef struct Fetch_info {
t_element * __RET__;
} Fetch_info;
/* End of info struct definition: Fetch */

/* ID function of interface: Fetch */
inline static call_id_t Fetch_id(void *info, thread_id_t __TID__) {
Fetch_info* theInfo = (Fetch_info*)info;
t_element * __RET__ = theInfo->__RET__;

call_id_t __ID__ = fetch_id ( __RET__ );
return __ID__;
}
/* End of ID function: Fetch */

/* Check action function of interface: Fetch */
inline static bool Fetch_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
bool check_passed;
Fetch_info* theInfo = (Fetch_info*)info;
t_element * __RET__ = theInfo->__RET__;

check_passed = fetch_check ( __RET__ );
if (!check_passed) return false;
fetch ( __RET__ , __TID__ ) ;
return true;
}
/* End of check action function: Fetch */

/* Definition of interface info struct: Prepare */
typedef struct Prepare_info {
t_element * __RET__;
} Prepare_info;
/* End of info struct definition: Prepare */

/* ID function of interface: Prepare */
inline static call_id_t Prepare_id(void *info, thread_id_t __TID__) {
Prepare_info* theInfo = (Prepare_info*)info;
t_element * __RET__ = theInfo->__RET__;

call_id_t __ID__ = prepare_id ( );
return __ID__;
}
/* End of ID function: Prepare */

/* Check action function of interface: Prepare */
inline static bool Prepare_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
bool check_passed;
Prepare_info* theInfo = (Prepare_info*)info;
t_element * __RET__ = theInfo->__RET__;

check_passed = prepare_check ( __RET__ , __TID__ );
if (!check_passed) return false;
prepare ( __ID__ , __RET__ , __TID__ ) ;
return true;
}
/* End of check action function: Prepare */

/* ID function of interface: Consume */
inline static call_id_t Consume_id(void *info, thread_id_t __TID__) {
call_id_t __ID__ = consume_id ( __TID__ );
return __ID__;
}
/* End of ID function: Consume */

/* Check action function of interface: Consume */
inline static bool Consume_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
bool check_passed;
check_passed = consume_check ( __TID__ );
if (!check_passed) return false;
consume ( __TID__ ) ;
return true;
}
/* End of check action function: Consume */

#define INTERFACE_SIZE 4
static void** func_ptr_table;
static anno_hb_init** hb_init_table;

/* Define function for sequential code initialization */
inline static void __sequential_init() {
/* Init func_ptr_table */
func_ptr_table = (void**) malloc(sizeof(void*) * 4 * 2);
func_ptr_table[2 * 3] = (void*) &Publish_id;
func_ptr_table[2 * 3 + 1] = (void*) &Publish_check_action;
func_ptr_table[2 * 0] = (void*) &Fetch_id;
func_ptr_table[2 * 0 + 1] = (void*) &Fetch_check_action;
func_ptr_table[2 * 2] = (void*) &Prepare_id;
func_ptr_table[2 * 2 + 1] = (void*) &Prepare_check_action;
func_ptr_table[2 * 1] = (void*) &Consume_id;
func_ptr_table[2 * 1 + 1] = (void*) &Consume_check_action;
/* Prepare(true) -> Fetch(true) */
struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
hbConditionInit0->interface_num_before = 2;
hbConditionInit0->hb_condition_num_before = 0;
hbConditionInit0->interface_num_after = 0;
hbConditionInit0->hb_condition_num_after = 0;
/* Publish(true) -> Consume(true) */
struct anno_hb_init *hbConditionInit1 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
hbConditionInit1->interface_num_before = 3;
hbConditionInit1->hb_condition_num_before = 0;
hbConditionInit1->interface_num_after = 1;
hbConditionInit1->hb_condition_num_after = 0;
/* Init hb_init_table */
hb_init_table = (anno_hb_init**) malloc(sizeof(anno_hb_init*) * 2);
#define HB_INIT_TABLE_SIZE 2
hb_init_table[0] = hbConditionInit0;
hb_init_table[1] = hbConditionInit1;
/* Pass init info, including function table info & HB rules */
struct anno_init *anno_init = (struct anno_init*) malloc(sizeof(struct anno_init));
anno_init->func_table = func_ptr_table;
anno_init->func_table_size = INTERFACE_SIZE;
anno_init->hb_init_table = hb_init_table;
anno_init->hb_init_table_size = HB_INIT_TABLE_SIZE;
struct spec_annotation *init = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
init->type = INIT;
init->annotation = anno_init;
cdsannotate(SPEC_ANALYSIS, init);

list = new_spec_list ( ) ;
tag = new_id_tag ( ) ;
}
/* End of Global construct generation in class */

t_element * read_fetch() {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 0;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
t_element * __RET__ = __wrapper__read_fetch();
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 0;
hb_condition->hb_condition_num = 0;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

Fetch_info* info = (Fetch_info*) malloc(sizeof(Fetch_info));
info->__RET__ = __RET__;
struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 0;
interface_end->info = info;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
return __RET__;
}
t_element * __wrapper__read_fetch() {
unsigned int rdwr = m_rdwr . load ( mo_acquire ) ;
/* Automatically generated code for potential commit point: Fetch_Potential_Point */

if (true) {
struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
potential_cp_define->label_num = 0;
struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
annotation_potential_cp_define->annotation = potential_cp_define;
cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
}
unsigned int rd , wr ;
for ( ; ; ) {
rd = ( rdwr >> 16 ) & 0xFFFF ; wr = rdwr & 0xFFFF ; if ( wr == rd ) {
/* Automatically generated code for commit point define check: Fetch_Empty_Point */

if (true) {
struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
cp_define->label_num = 1;
cp_define->potential_cp_label_num = 0;
struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define->type = CP_DEFINE;
annotation_cp_define->annotation = cp_define;
cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
}
return false ;
}
bool succ = m_rdwr . compare_exchange_weak ( rdwr , rdwr + ( 1 << 16 ) , mo_acq_rel ) ;
/* Automatically generated code for commit point define check: Fetch_Succ_Point */

if (succ == true) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 2;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
}
if ( succ ) break ;
else thrd_yield ( ) ;
}
rl :: backoff bo ;
while ( ( m_written . load ( mo_acquire ) & 0xFFFF ) != wr ) {
thrd_yield ( ) ;
}
t_element * p = & ( m_array [ rd % t_size ] ) ;
return p ;
}

void read_consume() {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 1;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
__wrapper__read_consume();
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 1;
hb_condition->hb_condition_num = 0;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 1;
interface_end->info = NULL;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}
void __wrapper__read_consume() {
m_read . fetch_add ( 1 , mo_release ) ;
/* Automatically generated code for commit point define check: Consume_Point */

if (true) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 3;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
}
}

t_element * write_prepare() {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 2;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
t_element * __RET__ = __wrapper__write_prepare();
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 2;
hb_condition->hb_condition_num = 0;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

Prepare_info* info = (Prepare_info*) malloc(sizeof(Prepare_info));
info->__RET__ = __RET__;
struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 2;
interface_end->info = info;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
return __RET__;
}
t_element * __wrapper__write_prepare() {
unsigned int rdwr = m_rdwr . load ( mo_acquire ) ;
/* Automatically generated code for potential commit point: Prepare_Potential_Point */

if (true) {
struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
potential_cp_define->label_num = 4;
struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
annotation_potential_cp_define->annotation = potential_cp_define;
cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
}
unsigned int rd , wr ;
for ( ; ; ) {
rd = ( rdwr >> 16 ) & 0xFFFF ; wr = rdwr & 0xFFFF ; if ( wr == ( ( rd + t_size ) & 0xFFFF ) ) {
/* Automatically generated code for commit point define check: Prepare_Full_Point */

if (true) {
struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
cp_define->label_num = 5;
cp_define->potential_cp_label_num = 4;
struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define->type = CP_DEFINE;
annotation_cp_define->annotation = cp_define;
cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
}
return NULL ;
}
bool succ = m_rdwr . compare_exchange_weak ( rdwr , ( rd << 16 ) | ( ( wr + 1 ) & 0xFFFF ) , mo_acq_rel ) ;
/* Automatically generated code for commit point define check: Prepare_Succ_Point */

if (succ == true) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 6;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
}
if ( succ ) break ;
else thrd_yield ( ) ;
}
rl :: backoff bo ;
while ( ( m_read . load ( mo_acquire ) & 0xFFFF ) != rd ) {
thrd_yield ( ) ;
}
t_element * p = & ( m_array [ wr % t_size ] ) ;
return p ;
}

void write_publish() {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 3;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
__wrapper__write_publish();
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 3;
hb_condition->hb_condition_num = 0;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 3;
interface_end->info = NULL;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}
void __wrapper__write_publish() {
m_written . fetch_add ( 1 , mo_release ) ;
/* Automatically generated code for commit point define check: Publish_Point */

if (true) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 7;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
}
}
}
;
template < typename t_element , size_t t_size >
void** mpmc_boundq_1_alt<t_element, t_size>::func_ptr_table;
template < typename t_element , size_t t_size >
anno_hb_init** mpmc_boundq_1_alt<t_element, t_size>::hb_init_table;
template < typename t_element , size_t t_size >
spec_list * mpmc_boundq_1_alt<t_element, t_size>::list;
template < typename t_element , size_t t_size >
id_tag_t * mpmc_boundq_1_alt<t_element, t_size>::tag;
