#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <CUnit/CUnit.h>
#include "cunit_tools.h"
#include <string.h>
#include <stdbool.h>
#include "../common/list.h"
#include <commons/string.h>

// ------------------ HELP FUNCTION'S ---------------------

typedef struct {
	char* algo;
	int borrar;
} t_test_struct;


static  t_test_struct* t_test_struct_create(char* algo, int borrar) {
	t_test_struct* new = malloc(sizeof(t_test_struct));
	new->algo = string_duplicate(algo);
	new->borrar = borrar;
	return new;
}


static void t_test_struct_destroy(t_test_struct* self) {
	free(self->algo);
	free(self);
}


t_test_struct *elems[3];

// --------------------------------------------------------

static int init_suite() {
	elems[0] = t_test_struct_create("elem1", 2);
	elems[1] = t_test_struct_create("elem2", 3);
	elems[2] = t_test_struct_create("elem3", 5);
	return 0;
}

static int clean_suite() {
	t_test_struct_destroy(elems[0]);
	t_test_struct_destroy(elems[1]);
	t_test_struct_destroy(elems[2]);
	return 0;
}

static void test_list_add() {
	t_list * list = list_create();
	t_test_struct *e1 = t_test_struct_create("elem1", 2);
	list_add(list, e1);
	list_add(list, t_test_struct_create("elem2", 3));

	t_test_struct *aux = list_get(list, 0);
	CU_ASSERT_PTR_NOT_NULL(aux);
	CU_ASSERT_STRING_EQUAL(aux->algo, "elem1");
	CU_ASSERT_EQUAL(aux->borrar, 2);

	aux = list_get(list, 1);
	CU_ASSERT_PTR_NOT_NULL(aux);
	CU_ASSERT_STRING_EQUAL(aux->algo, "elem2");
	CU_ASSERT_EQUAL(aux->borrar, 3);

	CU_ASSERT_EQUAL(list_size(list), 2);

	list_destroy_and_destroy_elements(list, (void*) t_test_struct_destroy);
}

static void test_list_remove() {
	t_list * list = list_create();

	t_test_struct *e1 = t_test_struct_create("elem3", 5);

	list_add(list, e1);
	list_add(list, t_test_struct_create("elem4", 8));

	t_test_struct *aux = list_remove(list, 0);
	CU_ASSERT_PTR_NOT_NULL( aux);
	CU_ASSERT_STRING_EQUAL( aux->algo, "elem3");
	CU_ASSERT_EQUAL( aux->borrar, 5);
	t_test_struct_destroy(aux);

	CU_ASSERT_EQUAL(list_size(list), 1);

	aux = list_get(list, 0);
	CU_ASSERT_PTR_NOT_NULL( aux);
	CU_ASSERT_STRING_EQUAL( aux->borrar, "elem4");
	CU_ASSERT_EQUAL( aux->borrar, 8);

	list_destroy_and_destroy_elements(list, (void*) t_test_struct_destroy);
}

static void test_list_remove_by_closure() {
	t_list * list = list_create();

	list_add(list, t_test_struct_create("elem1", 1));
	list_add(list, t_test_struct_create("elem2", 1));
	list_add(list, t_test_struct_create("elem3", 2));
	list_add(list, t_test_struct_create("elem4", 3));
	list_add(list, t_test_struct_create("elem5", 5));

	bool _borrar(t_test_struct *e) {
		return e->borrar==2;
	}

	t_test_struct *aux = list_remove_by_condition(list, (void*) _borrar);
	CU_ASSERT_PTR_NOT_NULL(aux);
	CU_ASSERT_STRING_EQUAL(aux->algo, "elem3");
	CU_ASSERT_EQUAL(aux->borrar, 2);
	t_test_struct_destroy(aux);

	CU_ASSERT_EQUAL(list_size(list), 4);

	list_destroy_and_destroy_elements(list, (void*) t_test_struct_destroy);
}



/**********************************************************************************************
 *  							Building the test for CUnit
 *********************************************************************************************/

static CU_TestInfo tests[] = {
		{ "Test Add List Element", test_list_add },
		{ "Test Remove List Element", test_list_remove },
		{ "Test Remove By Closure List Element", test_list_remove_by_closure },
		CU_TEST_INFO_NULL, };



CUNIT_MAKE_SUITE(list, "Test List TAD", init_suite, clean_suite, tests);
