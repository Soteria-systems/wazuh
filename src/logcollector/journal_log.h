#ifndef w_journal_H
#define w_journal_H

#include <shared.h>

#include "cJSON.h"
#include "expression.h"

/*******************************************************************************
 * NOTE: This module is not thread-safe.
 *
 * All functions listed here are thread-agnostic and only a single specific
 * thread may operate on a given object during its entire lifetime.
 * It's safe to allocate multiple independent objects and use each from a
 * specific thread in parallel.
 * However, it's not safe to allocate such an object in one thread, and operate
 * or free it from any other, even if locking is used to ensure these threads
 * don't operate on it at the very same time.
 *******************************************************************************/
/**********************************************************
 *                    Library related
 ***********************************************************/
typedef struct sd_journal sd_journal;
typedef int (*sd_journal_open_t)(sd_journal **ret, int flags);
typedef void (*sd_journal_close_t)(sd_journal *j);
typedef int (*sd_journal_get_realtime_usec_t)(sd_journal *j, uint64_t *ret);
typedef int (*sd_journal_seek_tail_t)(sd_journal *j);
typedef int (*sd_journal_previous_t)(sd_journal *j);
typedef int (*sd_journal_seek_realtime_usec_t)(sd_journal *j, uint64_t usec);
typedef int (*sd_journal_next_t)(sd_journal *j);
typedef int (*sd_journal_get_cutoff_realtime_usec_t)(sd_journal *j, uint64_t *from, uint64_t *to);
typedef int (*sd_journal_enumerate_available_data_t)(sd_journal *j, const void **data, size_t *l);
typedef int (*sd_journal_get_data_t)(sd_journal *j, const char *field, const void **data, size_t *l);

typedef struct {
    sd_journal_open_t open;
    sd_journal_close_t close;
    sd_journal_get_realtime_usec_t get_realtime_usec;
    sd_journal_seek_tail_t seek_tail;
    sd_journal_previous_t previous;
    sd_journal_seek_realtime_usec_t seek_realtime_usec;
    sd_journal_next_t next;
    sd_journal_get_cutoff_realtime_usec_t get_cutoff_realtime_usec;
    sd_journal_enumerate_available_data_t enumerate_available_data;
    sd_journal_get_data_t get_data;
    void* handle;
} w_sd_journal_lib_t;



/**********************************************************
 *                    Context related
 ***********************************************************/

/**
 * @brief Journal log context
 */
typedef struct
{
    w_sd_journal_lib_t* lib;            ///< Journal functions
    sd_journal* journal;              ///< Journal context
    uint64_t timestamp;               ///< Last timestamp processed (__REALTIME_TIMESTAMP)
} w_journal_context_t;

/**
 * @brief Get a new journal log context
 *
 * The caller is responsible for freeing the returned context.
 * @param ctx Journal log context
 * @return int 0 on success or a negative errno-style error code.
 * @note The context should be created and used by a single thread only.
 */
int w_journal_context_create(w_journal_context_t** ctx);

/**
 * @brief Free the journal log context and all its resources
 *
 * The context pointer is invalid after the call.
 * @param ctx Journal log context
 */
void w_journal_context_free(w_journal_context_t* ctx);

/**
 * @brief Try update the timestamp in the journal log context with the timestamp of the current entry
 *
 * If failed to get the timestamp, the timestamp updated with the current time.
 * @param ctx Journal log context
 */
void w_journal_context_update_timestamp(w_journal_context_t* ctx);

/**
 * @brief Move the cursor to the most recent entry
 *
 * @param ctx Journal log context
 * @return int 0 on success or a negative errno-style error code.
 * @note This function is not thread-safe.
 *
 */
int w_journal_context_seek_most_recent(w_journal_context_t* ctx);

/**
 * @brief Move the cursor to the entry with the specified timestamp or the next newer entry available.
 *
 * If the timestamp is in the future or 0, the cursor is moved most recent entry.
 * If the timestamp is older than the oldest available entry, the cursor is moved to the oldest entry.
 * @param ctx Journal log context
 * @param timestamp The timestamp to seek
 * @return int 0 on success or a negative errno-style error code.
 */
int w_journal_context_seek_timestamp(w_journal_context_t* ctx, uint64_t timestamp);

/**
 * @brief Move the cursor to the next newest entry
 *
 * @param ctx Journal log context
 * @return int 0 no more entries or a negative errno-style error code.
 * @note This function is not thread-safe.
 */
int w_journal_context_next_newest(w_journal_context_t* ctx);

/**
 * @brief Get the oldest accessible timestamp in the journal (__REALTIME_TIMESTAMP)
 * 
 * @param ctx Journal log context
 * @param timestamp The oldest timestamp
 * @return int 0 on success or a negative errno-style error code.
 * @note This function is not thread-safe.
 */
int w_journal_context_get_oldest_timestamp(w_journal_context_t* ctx, uint64_t* timestamp);

/**********************************************************
 *                   Entry related
 **********************************************************/
/**
 * @brief Determine the types of dump of a journal log entry
 */
typedef enum
{
    W_JOURNAL_ENTRY_DUMP_TYPE_INVALID = -1, ///< Invalid dump type
    W_JOURNAL_ENTRY_DUMP_TYPE_JSON,         ///< JSON dump
    W_JOURNAL_ENTRY_DUMP_TYPE_SYSLOG,       ///< Syslog dump
} w_journal_entry_dump_type_t;

/**
 * @brief Represents a dump of a journal log entry
 */
typedef struct
{
    w_journal_entry_dump_type_t type; ///< Dump type
    union
    {
        cJSON* json;    ///< JSON dump
        char* syslog;   ///< Syslog dump
    } data;             ///< Dump data
    uint64_t timestamp; ///< Indexing timestamp (__REALTIME_TIMESTAMP)
} w_journal_entry_t;

/**
 * @brief Create the entry from the current entry in the journal log context
 *
 * The caller is responsible for freeing the returned entry.
 * @param ctx Journal log context
 * @param type The type of dump
 * @return w_journal_entry_t* The current entry or NULL on error
 * @note This function is not thread-safe.
 */
w_journal_entry_t* w_journal_entry_dump(w_journal_context_t* ctx, w_journal_entry_dump_type_t type);

/**
 * @brief Free the entry and all its resources,
 *
 * The entry pointer is invalid after the call.
 * @param entry Journal log entry
 */
void w_journal_entry_free(w_journal_entry_t* entry);

/**
 * @brief Dump the current entry to a string representation
 *
 * The caller is responsible for freeing the returned string.
 * @param entry Journal log entry
 * @return char*  The string representation of the entry or NULL on error
 */
char* w_journal_entry_to_string(w_journal_entry_t* entry);

/**********************************************************
 *                   Filter related
 **********************************************************/

/**
 * @brief Represents a filter unit, the minimal condition of a filter
 *
 */
typedef struct _w_journal_filter_unit_t
{
    char* field;           // Field to try match
    w_expression_t* exp; // Expression to match against the field (PCRE2)
    int ignore_if_missing; // Ignore if the field is missing (TODO: Use BOOL)
} _w_journal_filter_unit_t;

/**
 * @brief Represents a filter, a set of filter units, all of which must match
 */
typedef struct w_journal_filter_t
{
    _w_journal_filter_unit_t** units; // Array of unit filter TODO Change to list
    size_t units_size;                // Number of units
} w_journal_filter_t;

/**
 * @brief Free the filter and all its resources
 *
 * The filter pointer is invalid after the call.
 */
void w_journal_filter_free(w_journal_filter_t* filter);

/**
 * @brief Add a condition to the filter, creating the filter if it does not exist
 *
 * The filter will be updated to add the new condition.
 * @param filter Journal log filter
 * @param field Field to try match
 * @param expression expression to match against the field (PCRE2)
 * @param ignore_if_missing Ignore if the field is missing
 * @return int 0 on success or non-zero on error
 */
int w_journal_filter_add_condition(w_journal_filter_t** filter, char* field, char* expression, int ignore_if_missing);

/**
 * @brief Apply the filter to the journal log context
 *
 * The filter will be applied to the journal log context.
 * @param ctx Journal log context
 * @param filter Journal log filter
 * @return int positive number of entries matched, 0 if no entries matched, or a negative errno-style error code.
 */
int w_journal_filter_apply(w_journal_context_t* ctx, w_journal_filter_t* filter);

#endif // w_journal_H
