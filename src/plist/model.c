/* ========================================================================= */
/**
 * @file model.c
 *
 * @copyright
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <libbase/libbase.h>
#include <libbase/plist.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* == Declarations ========================================================= */

/** Base type of a config object. */
struct _bspl_object_t {
    /** Type of this object. */
    bspl_type_t type;
    /** References held to this object. */
    int references;
    /** Abstract virtual method: Destroys the object. */
    void (*destroy_fn)(bspl_object_t *object_ptr);
};

/** Config object: A string. */
struct _bspl_string_t {
    /** The string's superclass: An object. */
    bspl_object_t           super_object;
    /** Value of the string. */
    char                      *value_ptr;
};

/** Config object: A dict. */
struct _bspl_dict_t {
    /** The dict's superclass: An object. */
    bspl_object_t           super_object;
    /** Implemented as a tree. Benefit: Keeps sorting order. */
    bs_avltree_t              *tree_ptr;
};

/** An item in the dict. Internal class. */
typedef struct {
    /** Node in the tree. */
    bs_avltree_node_t         avlnode;
    /** The lookup key. */
    char                      *key_ptr;
    /** The value, is an object. */
    bspl_object_t           *value_object_ptr;
} bspl_dict_item_t;

/** Array object. A vector of pointers to the objects. */
struct _bspl_array_t {
    /** The array's superclass: An object. */
    bspl_object_t           super_object;
    /** Vector holding pointers to each object. */
    bs_ptr_vector_t           object_vector;
};

static bool _bspl_object_init(
    bspl_object_t *object_ptr,
    bspl_type_t type,
    void (*destroy_fn)(bspl_object_t *object_ptr));

static void _bspl_string_object_destroy(bspl_object_t *object_ptr);
static void _bspl_dict_object_destroy(bspl_object_t *object_ptr);
static void _bspl_array_object_destroy(bspl_object_t *object_ptr);

static bspl_dict_item_t *_bspl_dict_item_create(
    const char *key_ptr,
    bspl_object_t *object_ptr);
static void _bspl_dict_item_destroy(
    bspl_dict_item_t *item_ptr);

static int _bspl_dict_item_node_cmp(
    const bs_avltree_node_t *node_ptr,
    const void *key_ptr);
static void _bspl_dict_item_node_destroy(
    bs_avltree_node_t *node_ptr);

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_object_ref(bspl_object_t *object_ptr)
{
    ++object_ptr->references;
    return object_ptr;
}

/* ------------------------------------------------------------------------- */
void bspl_object_unref(bspl_object_t *object_ptr)
{
    if (NULL == object_ptr) return;

    // Check references. Warn on potential double-free.
    BS_ASSERT(0 < object_ptr->references);
    if (0 < --object_ptr->references) return;

    object_ptr->destroy_fn(object_ptr);
}

/* ------------------------------------------------------------------------- */
bspl_type_t bspl_object_type(bspl_object_t *object_ptr)
{
    return object_ptr->type;
}

/* ------------------------------------------------------------------------- */
bspl_string_t *bspl_string_create(const char *value_ptr)
{
    BS_ASSERT(NULL != value_ptr);
    bspl_string_t *string_ptr = logged_calloc(1, sizeof(bspl_string_t));
    if (NULL == string_ptr) return NULL;

    if (!_bspl_object_init(&string_ptr->super_object,
                             BSPL_STRING,
                             _bspl_string_object_destroy)) {
        free(string_ptr);
        return NULL;
    }

    string_ptr->value_ptr = logged_strdup(value_ptr);
    if (NULL == string_ptr->value_ptr) {
        _bspl_string_object_destroy(bspl_object_from_string(string_ptr));
        return NULL;
    }

    return string_ptr;
}

/* ------------------------------------------------------------------------- */
const char *bspl_string_value(const bspl_string_t *string_ptr)
{
    if (NULL == string_ptr) return NULL;  // Guard clause.
    return string_ptr->value_ptr;
}

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_object_from_string(bspl_string_t *string_ptr)
{
    if (NULL == string_ptr) return NULL;
    return &string_ptr->super_object;
}

/* ------------------------------------------------------------------------- */
bspl_string_t *bspl_string_from_object(bspl_object_t *object_ptr)
{
    if (NULL == object_ptr || BSPL_STRING != object_ptr->type) return NULL;
    return BS_CONTAINER_OF(object_ptr, bspl_string_t, super_object);
}

/* ------------------------------------------------------------------------- */
bspl_dict_t *bspl_dict_create(void)
{
    bspl_dict_t *dict_ptr = logged_calloc(1, sizeof(bspl_dict_t));
    if (NULL == dict_ptr) return NULL;

    if (!_bspl_object_init(&dict_ptr->super_object,
                             BSPL_DICT,
                             _bspl_dict_object_destroy)) {
        free(dict_ptr);
        return NULL;
    }

    dict_ptr->tree_ptr = bs_avltree_create(
        _bspl_dict_item_node_cmp,
        _bspl_dict_item_node_destroy);
    if (NULL == dict_ptr->tree_ptr) {
        _bspl_dict_object_destroy(bspl_object_from_dict(dict_ptr));
        return NULL;
    }

    return dict_ptr;
}

/* ------------------------------------------------------------------------- */
bool bspl_dict_add(
    bspl_dict_t *dict_ptr,
    const char *key_ptr,
    bspl_object_t *object_ptr)
{
    bspl_dict_item_t *item_ptr = _bspl_dict_item_create(
        key_ptr, object_ptr);
    if (NULL == item_ptr) return false;

    bool rv = bs_avltree_insert(
        dict_ptr->tree_ptr, item_ptr->key_ptr, &item_ptr->avlnode, false);
    if (!rv) _bspl_dict_item_destroy(item_ptr);
    return rv;
}

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_dict_get(
    bspl_dict_t *dict_ptr,
    const char *key_ptr)
{
    if (NULL == dict_ptr) return NULL;
    bs_avltree_node_t *node_ptr = bs_avltree_lookup(
        dict_ptr->tree_ptr, key_ptr);
    if (NULL == node_ptr) return NULL;

    bspl_dict_item_t *item_ptr = BS_CONTAINER_OF(
        node_ptr, bspl_dict_item_t, avlnode);
    return item_ptr->value_object_ptr;
}

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_object_from_dict(bspl_dict_t *dict_ptr)
{
    if (NULL == dict_ptr) return NULL;
    return &dict_ptr->super_object;
}

/* ------------------------------------------------------------------------- */
bspl_dict_t *bspl_dict_from_object(bspl_object_t *object_ptr)
{
    if (NULL == object_ptr || BSPL_DICT != object_ptr->type) return NULL;
    return BS_CONTAINER_OF(object_ptr, bspl_dict_t, super_object);
}

/* ------------------------------------------------------------------------- */
bool bspl_dict_foreach(
    bspl_dict_t *dict_ptr,
    bool (*fn)(const char *key_ptr,
               bspl_object_t *object_ptr,
               void *userdata_ptr),
    void *userdata_ptr)
{
    for (bs_avltree_node_t *node_ptr = bs_avltree_min(dict_ptr->tree_ptr);
         NULL != node_ptr;
         node_ptr = bs_avltree_node_next(dict_ptr->tree_ptr, node_ptr)) {
        bspl_dict_item_t *item_ptr = BS_CONTAINER_OF(
            node_ptr, bspl_dict_item_t, avlnode);
        if (!fn(item_ptr->key_ptr, item_ptr->value_object_ptr, userdata_ptr)) {
            return false;
        }
    }
    return true;
}

/* == Array methods ======================================================== */

/* ------------------------------------------------------------------------- */
bspl_array_t *bspl_array_create(void)
{
    bspl_array_t *array_ptr = logged_calloc(1, sizeof(bspl_array_t));
    if (NULL == array_ptr) return NULL;

    if (!_bspl_object_init(&array_ptr->super_object,
                             BSPL_ARRAY,
                             _bspl_array_object_destroy)) {
        free(array_ptr);
        return NULL;
    }

    if (!bs_ptr_vector_init(&array_ptr->object_vector)) {
        _bspl_array_object_destroy(bspl_object_from_array(array_ptr));
        return NULL;
    }
    return array_ptr;
}

/* ------------------------------------------------------------------------- */
bool bspl_array_push_back(
    bspl_array_t *array_ptr,
    bspl_object_t *object_ptr)
{
    bspl_object_t *new_object_ptr = bspl_object_ref(object_ptr);
    if (NULL == new_object_ptr) return false;

    return bs_ptr_vector_push_back(&array_ptr->object_vector, new_object_ptr);
}

/* ------------------------------------------------------------------------- */
size_t bspl_array_size(bspl_array_t *array_ptr)
{
    return bs_ptr_vector_size(&array_ptr->object_vector);
}

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_array_at(
    bspl_array_t *array_ptr,
    size_t index)
{
    return bs_ptr_vector_at(&array_ptr->object_vector, index);
}

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_object_from_array(bspl_array_t *array_ptr)
{
    if (NULL == array_ptr) return NULL;
    return &array_ptr->super_object;
}

/* ------------------------------------------------------------------------- */
bspl_array_t *bspl_array_from_object(bspl_object_t *object_ptr)
{
    if (NULL == object_ptr || BSPL_ARRAY != object_ptr->type) return NULL;
    return BS_CONTAINER_OF(object_ptr, bspl_array_t, super_object);
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/**
 * Initializes the object base class.
 *
 * @param object_ptr
 * @param type
 * @param destroy_fn
 *
 * @return true on success.
 */
bool _bspl_object_init(
    bspl_object_t *object_ptr,
    bspl_type_t type,
    void (*destroy_fn)(bspl_object_t *object_ptr))
{
    BS_ASSERT(NULL != object_ptr);
    BS_ASSERT(NULL != destroy_fn);
    object_ptr->type = type;
    object_ptr->destroy_fn = destroy_fn;
    object_ptr->references = 1;
    return true;
}

/* ------------------------------------------------------------------------- */
/**
 * Implementation of @ref bspl_object_t::destroy_fn. Destroys the string.
 *
 * @param object_ptr
 */
void _bspl_string_object_destroy(bspl_object_t *object_ptr)
{
    bspl_string_t *string_ptr = BS_ASSERT_NOTNULL(
        bspl_string_from_object(object_ptr));

    if (NULL != string_ptr->value_ptr) {
        free(string_ptr->value_ptr);
        string_ptr->value_ptr = NULL;
    }

    free(string_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Implementation of @ref bspl_object_t::destroy_fn. Destroys the dict,
 * including all contained elements.
 *
 * @param object_ptr
 */
void _bspl_dict_object_destroy(bspl_object_t *object_ptr)
{
    bspl_dict_t *dict_ptr = BS_ASSERT_NOTNULL(
        bspl_dict_from_object(object_ptr));

    if (NULL != dict_ptr->tree_ptr) {
        bs_avltree_destroy(dict_ptr->tree_ptr);
        dict_ptr->tree_ptr = NULL;
    }
    free(dict_ptr);
}

/* ------------------------------------------------------------------------- */
/** Ctor: Creates a dict item. Copies the key, and duplicates the object. */
bspl_dict_item_t *_bspl_dict_item_create(
    const char *key_ptr,
    bspl_object_t *object_ptr)
{
    bspl_dict_item_t *item_ptr = logged_calloc(
        1, sizeof(bspl_dict_item_t));
    if (NULL == item_ptr) return NULL;

    item_ptr->key_ptr = logged_strdup(key_ptr);
    if (NULL == item_ptr->key_ptr) {
        _bspl_dict_item_destroy(item_ptr);
        return NULL;
    }

    item_ptr->value_object_ptr = bspl_object_ref(object_ptr);
    if (NULL == item_ptr->value_object_ptr) {
        _bspl_dict_item_destroy(item_ptr);
        return NULL;
    }
    return item_ptr;
}

/* ------------------------------------------------------------------------- */
/** Dtor: Destroys the dict item. */
void _bspl_dict_item_destroy(bspl_dict_item_t *item_ptr)
{
    if (NULL != item_ptr->value_object_ptr) {
        bspl_object_unref(item_ptr->value_object_ptr);
        item_ptr->value_object_ptr = NULL;
    }
    if (NULL != item_ptr->key_ptr) {
        free(item_ptr->key_ptr);
        item_ptr->key_ptr = NULL;
    }
    free(item_ptr);
}

/* ------------------------------------------------------------------------- */
/** Comparator for the dictionary item with another key. */
int _bspl_dict_item_node_cmp(const bs_avltree_node_t *node_ptr,
                               const void *key_ptr)
{
    bspl_dict_item_t *item_ptr = BS_CONTAINER_OF(
        node_ptr, bspl_dict_item_t, avlnode);
    return strcmp(item_ptr->key_ptr, key_ptr);
}

/* ------------------------------------------------------------------------- */
/** Destroy dict item by avlnode. Forward to @ref _bspl_dict_item_destroy. */
void _bspl_dict_item_node_destroy(bs_avltree_node_t *node_ptr)
{
    bspl_dict_item_t *item_ptr = BS_CONTAINER_OF(
        node_ptr, bspl_dict_item_t, avlnode);
    _bspl_dict_item_destroy(item_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Implementation of @ref bspl_object_t::destroy_fn. Destroys the array,
 * including all contained elements.
 *
 * @param object_ptr
 */
void _bspl_array_object_destroy(bspl_object_t *object_ptr)
{
    bspl_array_t *array_ptr = BS_ASSERT_NOTNULL(
        bspl_array_from_object(object_ptr));

    while (0 < bs_ptr_vector_size(&array_ptr->object_vector)) {
        bspl_object_t *object_ptr = bs_ptr_vector_at(
            &array_ptr->object_vector,
            bs_ptr_vector_size(&array_ptr->object_vector) - 1);
        bs_ptr_vector_erase(
            &array_ptr->object_vector,
            bs_ptr_vector_size(&array_ptr->object_vector) - 1);
        bspl_object_unref(object_ptr);
    }

    bs_ptr_vector_fini(&array_ptr->object_vector);

    free(array_ptr);
}

/* == Unit tests =========================================================== */

static void test_string(bs_test_t *test_ptr);
static void test_dict(bs_test_t *test_ptr);
static void test_array(bs_test_t *test_ptr);

const bs_test_case_t bspl_model_test_cases[] = {
    { 1, "string", test_string },
    { 1, "dict", test_dict },
    { 1, "array", test_array },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
/** Test helper: A callback for @ref bspl_dict_foreach. */
static bool foreach_callback(
    const char *key_ptr,
    __UNUSED__ bspl_object_t *object_ptr,
    void *userdata_ptr)
{
    int *map = userdata_ptr;
    if (strcmp(key_ptr, "key0") == 0) {
        *map |= 1;
    } else if (strcmp(key_ptr, "key1") == 0) {
        *map |= 2;
    }
    return true;
}

/* ------------------------------------------------------------------------- */
/** Tests the bspl_string_t methods. */
void test_string(bs_test_t *test_ptr)
{
    bspl_string_t *string_ptr;

    string_ptr = bspl_string_create("a test");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, string_ptr);
    BS_TEST_VERIFY_STREQ(test_ptr, "a test", string_ptr->value_ptr);

    bspl_object_t *object_ptr = bspl_object_from_string(string_ptr);
    BS_TEST_VERIFY_EQ(
        test_ptr,
        string_ptr,
        bspl_string_from_object(object_ptr));

    BS_TEST_VERIFY_EQ(
        test_ptr,
        BSPL_STRING,
        bspl_object_type(object_ptr));

    bspl_object_unref(object_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests the bspl_dict_t methods. */
void test_dict(bs_test_t *test_ptr)
{
    bspl_dict_t *dict_ptr = bspl_dict_create();

    bspl_object_t *obj0_ptr = BS_ASSERT_NOTNULL(
        bspl_object_from_string(bspl_string_create("val0")));
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bspl_dict_add(dict_ptr, "key0", obj0_ptr));
    bspl_object_unref(obj0_ptr);

    bspl_object_t *obj1_ptr = BS_ASSERT_NOTNULL(
        bspl_object_from_string(bspl_string_create("val1")));
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        bspl_dict_add(dict_ptr, "key0", obj1_ptr));
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bspl_dict_add(dict_ptr, "key1", obj1_ptr));
    bspl_object_unref(obj1_ptr);

    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "val0",
        bspl_string_value(bspl_string_from_object(
                                bspl_dict_get(dict_ptr, "key0"))));
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "val1",
        bspl_string_value(bspl_string_from_object(
                                bspl_dict_get(dict_ptr, "key1"))));

    int val = 0;
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bspl_dict_foreach(dict_ptr, foreach_callback, &val));
    BS_TEST_VERIFY_EQ(test_ptr, 3, val);

    BS_TEST_VERIFY_EQ(
        test_ptr,
        BSPL_DICT,
        bspl_object_type(bspl_object_from_dict(dict_ptr)));

    bspl_dict_unref(dict_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests the bspl_array_t methods. */
void test_array(bs_test_t *test_ptr)
{
    bspl_array_t *array_ptr = bspl_array_create();

    bspl_object_t *obj0_ptr = BS_ASSERT_NOTNULL(
        bspl_object_from_string(bspl_string_create("val0")));
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bspl_array_push_back(array_ptr, obj0_ptr));
    bspl_object_unref(obj0_ptr);

    bspl_object_t *obj1_ptr = BS_ASSERT_NOTNULL(
        bspl_object_from_string(bspl_string_create("val1")));
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bspl_array_push_back(array_ptr, obj1_ptr));
    bspl_object_unref(obj1_ptr);

    BS_TEST_VERIFY_EQ(test_ptr, obj0_ptr, bspl_array_at(array_ptr, 0));
    BS_TEST_VERIFY_EQ(test_ptr, obj1_ptr, bspl_array_at(array_ptr, 1));

    BS_TEST_VERIFY_EQ(
        test_ptr,
        BSPL_ARRAY,
        bspl_object_type(bspl_object_from_array(array_ptr)));

    bspl_array_unref(array_ptr);
}

/* == End of model.c ======================================================= */
