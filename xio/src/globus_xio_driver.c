#include "globus_xio.h"
#include "globus_i_xio.h"

void
globus_l_xio_op_restarted(
    globus_i_xio_op_t *                     op)
{
    globus_bool_t                           destroy_handle = GLOBUS_FALSE;
    globus_i_xio_context_t *                context;
    globus_i_xio_handle_t *                 handle;
    GlobusXIOName(globus_l_xio_op_restarted);

    GlobusXIODebugInternalEnter();

    context = op->_op_context;
    handle = op->_op_handle;
    globus_mutex_lock(&context->mutex);
    {
        GlobusXIOOpDec(op);
        if(op->ref == 0)
        {
            globus_i_xio_op_destroy(op, &destroy_handle);
        }
    }
    globus_mutex_unlock(&context->mutex);

    if(destroy_handle)
    {
        globus_i_xio_handle_destroy(handle);
    }
                                                                                
    GlobusXIODebugInternalExit();
}

globus_result_t
globus_i_xio_repass_write(
    globus_i_xio_op_t *                     op)
{
    globus_i_xio_op_entry_t *               my_op;
    globus_i_xio_context_entry_t *          next_context;
    globus_result_t                         res;
    globus_xio_iovec_t *                    tmp_iovec;
    int                                     iovec_count;
    GlobusXIOName(globus_i_xio_repass_write);

    GlobusXIODebugInternalEnter();

    my_op = &op->entry[op->ndx - 1];
    next_context = &op->_op_context->entry[op->ndx - 1];

    /* allocate tmp iovec to the bigest it could ever be */
    if(my_op->_op_ent_fake_iovec == NULL)
    {
        my_op->_op_ent_fake_iovec = (globus_xio_iovec_t *)
            globus_malloc(sizeof(globus_xio_iovec_t) *
                my_op->_op_ent_iovec_count);
    }
    tmp_iovec = my_op->_op_ent_fake_iovec;

    GlobusIXIOUtilTransferAdjustedIovec(
        tmp_iovec, iovec_count,
        my_op->_op_ent_iovec, my_op->_op_ent_iovec_count,
        my_op->_op_ent_nbytes);

    /* repass the operation down */
    res = next_context->driver->write_func(
            next_context->driver_handle,
            tmp_iovec,
            iovec_count,
            op);

    GlobusXIODebugInternalExit();

    return res;
}

globus_result_t
globus_i_xio_repass_read(
    globus_i_xio_op_t *                     op)
{
    globus_i_xio_op_entry_t *               my_op;
    globus_i_xio_context_entry_t *          next_context;
    globus_result_t                         res;
    globus_xio_iovec_t *                    tmp_iovec;
    int                                     iovec_count;
    GlobusXIOName(globus_i_xio_repass_read);

    GlobusXIODebugInternalEnter();

    my_op = &op->entry[op->ndx - 1];
    next_context = &op->_op_context->entry[op->ndx - 1];

    /* allocate tmp iovec to the bigest it could ever be */
    if(my_op->_op_ent_fake_iovec == NULL)
    {
        my_op->_op_ent_fake_iovec = (globus_xio_iovec_t *)
            globus_malloc(sizeof(globus_xio_iovec_t) *
                my_op->_op_ent_iovec_count);
    }
    tmp_iovec = my_op->_op_ent_fake_iovec;

    GlobusIXIOUtilTransferAdjustedIovec(
        tmp_iovec, iovec_count,
        my_op->_op_ent_iovec, my_op->_op_ent_iovec_count,
        my_op->_op_ent_nbytes);

    /* repass the operation down */
    res = next_context->driver->read_func(
            next_context->driver_handle,
            tmp_iovec,
            iovec_count,
            op);

    GlobusXIODebugInternalExit();

    return res;
}

void 
globus_i_xio_pass_failed(
    globus_i_xio_op_t *                     op,
    globus_i_xio_context_entry_t *          my_context,
    globus_bool_t *                         close,
    globus_bool_t *                         destroy_handle)
{
    GlobusXIOName(globus_i_xio_pass_failed);

    GlobusXIODebugInternalEnter();

    my_context->outstanding_operations--;
    /*there is an off chance that we could need to close here*/
    if((my_context->state == GLOBUS_XIO_CONTEXT_STATE_CLOSING ||
        my_context->state ==
        GLOBUS_XIO_CONTEXT_STATE_EOF_DELIVERED_AND_CLOSING) &&
        my_context->outstanding_operations == 0 &&
        !my_context->close_started)
    {
        globus_assert(my_context->close_op != NULL);
        *close = GLOBUS_TRUE;
    }

    op->ndx = op->entry[op->ndx - 1].prev_ndx;

    GlobusXIOOpDec(op);
    if(op->ref == 0)
    {
        globus_i_xio_op_destroy(op, destroy_handle);
    }

    GlobusXIODebugInternalExit();
}

void
globus_i_xio_handle_destroy(
    globus_i_xio_handle_t *                 handle)
{
    globus_bool_t                           destroy_context = GLOBUS_FALSE;
    GlobusXIOName(globus_i_xio_handle_destroy);

    GlobusXIODebugInternalEnter();

    globus_mutex_lock(&globus_l_mutex);
    {
        globus_mutex_lock(&handle->context->mutex);
        {
            handle->context->ref--;
            if(handle->context->ref == 0)
            {
                GlobusXIODebugPrintf(
                    GLOBUS_XIO_DEBUG_INFO,
                    ("[globus_i_xio_handle_destroy] :: context->ref == 0.\n"));
                destroy_context = GLOBUS_TRUE;
            }

            if(handle->sd_monitor != NULL)
            {
                GlobusXIODebugPrintf(
                    GLOBUS_XIO_DEBUG_INFO,
                        ("[globus_i_xio_handle_destroy]"
                        " :: signalling handle unload.\n"));

                handle->sd_monitor->count--;
                handle->sd_monitor = NULL;
                globus_cond_signal(&globus_l_cond);
            }
            else
            {
                globus_list_remove(&globus_l_outstanding_handles_list,
                    globus_list_search(
                        globus_l_outstanding_handles_list, handle));
            }
        }
        globus_mutex_unlock(&handle->context->mutex);

    }
    globus_mutex_unlock(&globus_l_mutex);

    if(destroy_context)
    {
        globus_i_xio_context_destroy(handle->context);
    }
    globus_assert(handle->ref == 0);
    globus_callback_space_destroy(handle->space);
    globus_free(handle);

    GlobusXIODebugInternalExit();
}

/* 
 *  called in the context lock
 */
void
globus_i_xio_handle_dec(
    globus_i_xio_handle_t *                 handle,
    globus_bool_t *                         destroy_handle)
{
    globus_result_t                         res;
    globus_i_xio_context_t *                context;
    globus_i_xio_space_info_t *             space_info;
    GlobusXIOName(globus_i_xio_handle_dec);

    GlobusXIODebugInternalEnter();

    context = handle->context;

    *destroy_handle = GLOBUS_FALSE;

    handle->ref--; 
    GlobusXIODebugPrintf(
        GLOBUS_XIO_DEBUG_INFO_VERBOSE,
        ("[globus_i_xio_handle_dec] :: handle ref at %d.\n", handle->ref));
    if(handle->ref == 0)
    {
        GlobusXIODebugPrintf(
            GLOBUS_XIO_DEBUG_INFO,
            ("[globus_i_xio_handle_dec] :: handle ref at 0.\n"));
        globus_assert(handle->state == GLOBUS_XIO_HANDLE_STATE_CLOSED);
        *destroy_handle = GLOBUS_TRUE;
        /* purge the ch list */
        while(!globus_list_empty(handle->cb_list))
        {
            space_info = (globus_i_xio_space_info_t *)
                globus_list_remove(&handle->cb_list, handle->cb_list);
            res = globus_callback_unregister(
                    space_info->ch,
                    NULL,
                    NULL,
                    NULL);
            if(res != GLOBUS_SUCCESS)
            {
                globus_panic(GLOBUS_XIO_MODULE, res, "failed to unregister");
            }
        }
    }

    GlobusXIODebugInternalExit();
}

/* 
 * called locked 
 */
void
globus_i_xio_op_destroy(
    globus_i_xio_op_t *                     op,
    globus_bool_t *                         destroy_handle)
{
    globus_i_xio_handle_t *                 handle;
    globus_i_xio_context_t *                context;
    int                                     ctr;
    GlobusXIOName(globus_i_xio_op_destroy);

    GlobusXIODebugInternalEnter();

    context = op->_op_context;
    handle = op->_op_handle;

    globus_assert(op->ref == 0);

    for(ctr = 0; ctr < op->stack_size; ctr++)
    {
        if(op->entry[ctr].dd != NULL)
        {
            op->_op_context->entry[ctr].driver->attr_destroy_func(
                op->entry[ctr].dd);
        }
    }

    globus_memory_push_node(&context->op_memory, op);
    if(handle != NULL)
    {
        globus_i_xio_handle_dec(handle, destroy_handle);
    }
    else
    {
        *destroy_handle = GLOBUS_FALSE;
    }
    GlobusXIODebugInternalExit();
}

void
globus_i_xio_driver_deliver_op(
    globus_i_xio_op_t *                     op,
    int                                     ndx,
    globus_xio_operation_type_t             deliver_type)
{
    GlobusXIOName(globus_i_xio_driver_deliver_op);

    GlobusXIODebugInternalEnter();

    switch(deliver_type)
    {
        case GLOBUS_XIO_OPERATION_TYPE_OPEN:
            globus_xio_driver_open_delivered(op, ndx, &deliver_type);
            break;

        case GLOBUS_XIO_OPERATION_TYPE_READ:
            globus_xio_driver_read_delivered(op, ndx, &deliver_type);
            break;

        case GLOBUS_XIO_OPERATION_TYPE_WRITE:
            globus_xio_driver_write_delivered(op, ndx, &deliver_type);
            break;

        default:
            globus_assert(0);
            break;
    }
    GlobusXIODebugInternalExit();
}

void
globus_i_xio_will_block_cb(
    globus_thread_callback_index_t          wb_ndx,
    globus_callback_space_t                 space,
    void *                                  user_args)
{
    globus_xio_operation_type_t             deliver_type;
    globus_i_xio_op_t *                     op;
    globus_i_xio_context_t *                context;
    int                                     ndx;
    GlobusXIOName(globus_i_xio_will_block_cb);

    GlobusXIODebugInternalEnter();

    op = (globus_i_xio_op_t *) user_args;

    globus_thread_blocking_callback_disable(&wb_ndx);

    context = op->_op_context;
    op->restarted = GLOBUS_TRUE;
    globus_assert(op->ndx == 0);
    ndx = op->ndx;
    while(ndx != op->stack_size)
    {
        globus_mutex_lock(&context->mutex);
        {
            if(op->entry[ndx].deliver_type != NULL)
            {
                GlobusXIOOpInc(op);
                deliver_type = *op->entry[ndx].deliver_type;
                *op->entry[ndx].deliver_type = GLOBUS_XIO_OPERATION_TYPE_NONE;
                op->entry[ndx].deliver_type = NULL;
            }
            else
            {
                deliver_type = GLOBUS_XIO_OPERATION_TYPE_NONE;
            }
        }
        globus_mutex_unlock(&context->mutex);

        switch(deliver_type)
        {
            case GLOBUS_XIO_OPERATION_TYPE_OPEN:
                globus_xio_driver_open_delivered(op, ndx, &deliver_type);
                break;

            case GLOBUS_XIO_OPERATION_TYPE_READ:
                globus_xio_driver_read_delivered(op, ndx, &deliver_type);
                break;

            case GLOBUS_XIO_OPERATION_TYPE_WRITE:
                globus_xio_driver_write_delivered(op, ndx, &deliver_type);
                break;

            case GLOBUS_XIO_OPERATION_TYPE_FINISHED:
                return;
                break;

            case GLOBUS_XIO_OPERATION_TYPE_NONE:
            case GLOBUS_XIO_OPERATION_TYPE_CLOSE:
                break;

            default:
                globus_assert(0);
                break;
        }

        ndx = op->entry[ndx].next_ndx;
        GlobusXIODebugPrintf(
            GLOBUS_XIO_DEBUG_INFO_VERBOSE,
           ("[%s:%d] :: Index = %d\n", _xio_name, __LINE__, ndx));
    }
    GlobusXIODebugInternalExit();
}

void
globus_l_xio_driver_op_write_kickout(
    void *                                  user_arg)
{
    globus_xio_operation_type_t             deliver_type;
    globus_xio_operation_type_t             op_type;
    int                                     ndx;
    int                                     wb_ndx;
    globus_i_xio_handle_t *                 handle;
    globus_i_xio_context_entry_t *          my_context;
    globus_i_xio_context_t *                context;
    globus_i_xio_op_entry_t *               my_op;
    globus_i_xio_op_t *                     op;
    GlobusXIOName(globus_l_xio_driver_op_write_kickout);

    GlobusXIODebugInternalEnter();
    op = (globus_i_xio_op_t *) user_arg;

    my_op = &op->entry[op->ndx - 1];
    op->entry[my_op->prev_ndx].next_ndx = op->ndx;
    op->ndx = my_op->prev_ndx;
    ndx = op->ndx;
    my_context = &op->_op_context->entry[ndx];
    handle = op->_op_handle;
    context = op->_op_context;

    /* see comment in globus_l_xio_driver_open_op_kickout */
    if(op->canceled == (op->start_ndx + 1))
    {
        op->canceled = 0;
    }

    /*
     *  before releasing the op back to the user we can safely set this 
     *  outside of a mutex.  Once the users callbcak is called the value
     *  on the local stack may be changed, theus the magic.
     */
    deliver_type = my_op->type;
    op_type = my_op->type;
    my_op->deliver_type = &deliver_type;

    if(ndx == 0)
    {
        /* at top level the callback should never be null */
        globus_assert(my_op->_op_ent_data_cb != NULL);
        globus_thread_blocking_space_callback_push(
            globus_i_xio_will_block_cb,
            (void *) op,
            op->blocking ? GLOBUS_CALLBACK_GLOBAL_SPACE: handle->space,
            &wb_ndx);

        my_op->_op_ent_data_cb(op, GlobusXIOObjToResult(op->cached_obj),
            my_op->_op_ent_nbytes, my_op->user_arg);
    
        globus_thread_blocking_callback_pop(&wb_ndx);
    }
    else
    {
        if(my_op->_op_ent_data_cb == NULL)
        {
            globus_xio_driver_finished_write(op, 
                GlobusXIOObjToResult(op->cached_obj), 
                my_op->_op_ent_nbytes);
        }
        else
        {
            my_op->_op_ent_data_cb(op, 
                GlobusXIOObjToResult(op->cached_obj),
                my_op->_op_ent_nbytes, my_op->user_arg);
        }
    }

    globus_xio_driver_write_delivered(op, ndx, &deliver_type);

    GlobusXIODebugInternalExit();
}   
   
void
globus_l_xio_driver_op_read_kickout(
    void *                                  user_arg)
{
    globus_xio_operation_type_t             deliver_type;
    globus_xio_operation_type_t             op_type;
    int                                     ndx;
    int                                     wb_ndx;
    globus_i_xio_handle_t *                 handle;
    globus_i_xio_context_entry_t *          my_context;
    globus_i_xio_context_t *                context;
    globus_i_xio_op_entry_t *               my_op;
    globus_i_xio_op_t *                     op;
    GlobusXIOName(globus_l_xio_driver_op_read_kickout);

    GlobusXIODebugInternalEnter();
    op = (globus_i_xio_op_t *) user_arg;

    my_op = &op->entry[op->ndx - 1];
    op->entry[my_op->prev_ndx].next_ndx = op->ndx;
    op->ndx = my_op->prev_ndx;
    ndx = op->ndx;
    my_context = &op->_op_context->entry[ndx];
    handle = op->_op_handle;
    context = op->_op_context;

    /* see comment in globus_l_xio_driver_open_op_kickout */
    if(op->canceled == (op->start_ndx + 1))
    {
        op->canceled = 0;
    }
    /*
     *  before releasing the op back to the user we can safely set this 
     *  outside of a mutex.  Once the users callbcak is called the value
     *  on the local stack may be changed, theus the magic.
     */
    deliver_type = my_op->type;
    op_type = my_op->type;
    my_op->deliver_type = &deliver_type;

    if(ndx == 0)
    {
        /* at top level the callback should never be null */
        globus_assert(my_op->_op_ent_data_cb != NULL);
        globus_thread_blocking_space_callback_push(
            globus_i_xio_will_block_cb,
            (void *) op,
            op->blocking ? GLOBUS_CALLBACK_GLOBAL_SPACE: handle->space,
            &wb_ndx);

        my_op->_op_ent_data_cb(op, GlobusXIOObjToResult(op->cached_obj),
            my_op->_op_ent_nbytes, my_op->user_arg);
    
        globus_thread_blocking_callback_pop(&wb_ndx);
    }
    else
    {
        if(my_op->_op_ent_data_cb == NULL)
        {
            globus_xio_driver_finished_read(op, 
                GlobusXIOObjToResult(op->cached_obj), 
                my_op->_op_ent_nbytes);
        }
        else
        {
            my_op->_op_ent_data_cb(op, 
                GlobusXIOObjToResult(op->cached_obj),
                my_op->_op_ent_nbytes, my_op->user_arg);
        }
    }

    globus_xio_driver_read_delivered(op, ndx, &deliver_type);

    GlobusXIODebugInternalExit();
}   
   
void
globus_l_xio_driver_purge_read_eof(
    globus_i_xio_context_entry_t *          my_context)
{
    globus_i_xio_op_t *                     tmp_op;
    GlobusXIOName(globus_l_xio_driver_purge_read_eof);

    GlobusXIODebugInternalEnter();
    while(!globus_list_empty(my_context->eof_op_list))
    {
        /* we can only get here if a eof has been received */ 
        globus_assert(my_context->state ==
            GLOBUS_XIO_CONTEXT_STATE_EOF_RECEIVED ||
            my_context->state ==
                GLOBUS_XIO_CONTEXT_STATE_EOF_DELIVERED ||
            my_context->state ==
                GLOBUS_XIO_CONTEXT_STATE_EOF_RECEIVED_AND_CLOSING ||
            my_context->state ==
                GLOBUS_XIO_CONTEXT_STATE_EOF_DELIVERED_AND_CLOSING);

        tmp_op = (globus_i_xio_op_t *)
                    globus_list_remove(&my_context->eof_op_list,
                        my_context->eof_op_list);

        globus_assert(tmp_op->entry[tmp_op->ndx - 1].type ==
            GLOBUS_XIO_OPERATION_TYPE_READ);
        globus_i_xio_register_oneshot(
            tmp_op->_op_handle,
            globus_l_xio_driver_op_read_kickout,
           (void *)tmp_op,
            tmp_op->blocking ? GLOBUS_CALLBACK_GLOBAL_SPACE: 
                                tmp_op->_op_handle->space);
    }
    GlobusXIODebugInternalExit();
}

globus_result_t
globus_i_xio_driver_start_close(
    globus_i_xio_op_t *                     op,
    globus_bool_t                           can_fail)
{
    globus_result_t                         res;
    globus_i_xio_handle_t *                 handle;
    globus_i_xio_op_entry_t *               my_op;
    globus_i_xio_context_t *                context;
    globus_i_xio_context_entry_t *          my_context;
    globus_bool_t                           destroy_handle = GLOBUS_FALSE;
    GlobusXIOName(globus_i_xio_driver_start_close);

    GlobusXIODebugInternalEnter();
    op->progress = GLOBUS_TRUE;
    op->block_timeout = GLOBUS_FALSE;
    my_op = &op->entry[op->ndx - 1];
    context = op->_op_context;
    handle = op->_op_handle;
    my_context = &context->entry[op->ndx - 1];

    globus_mutex_lock(&context->mutex);
    {
        GlobusXIOOpInc(op);
    }
    globus_mutex_unlock(&context->mutex);

    GlobusXIODebugPrintf(
        GLOBUS_XIO_DEBUG_INFO,
       ("[%s:%d] :: Index = %d\n", _xio_name, __LINE__, op->ndx));
    my_op->in_register = GLOBUS_TRUE;
    res = my_context->driver->close_func(
                    my_context->driver_handle,
                    my_op->attr,
                    my_context,
                    op);
    my_op->in_register = GLOBUS_FALSE;

    if(my_context->driver->attr_destroy_func != NULL && my_op->attr != NULL)
    {
        my_context->driver->attr_destroy_func(my_op->attr);
    }

    if(res != GLOBUS_SUCCESS && !can_fail)
    {
        globus_xio_driver_finished_close(op, res);
    }

    globus_mutex_lock(&context->mutex);
    {
        GlobusXIOOpDec(op);
        if(op->ref == 0)
        {
            globus_i_xio_op_destroy(op, &destroy_handle);
        }
    }
    globus_mutex_unlock(&context->mutex);

    if(destroy_handle)
    {
        globus_i_xio_handle_destroy(handle);
    }

    GlobusXIODebugInternalExit();
    return res;
}

/*
 *  driver callback kickout
 *
 *  when in a register the finish function kicks this out as a oneshot
 */
void
globus_l_xio_driver_op_close_kickout(
    void *                                  user_arg)
{
    globus_i_xio_op_t *                     op;
    globus_i_xio_op_entry_t *               my_op;
    GlobusXIOName(globus_l_xio_driver_op_kickout);

    GlobusXIODebugInternalEnter();
    op = (globus_i_xio_op_t *) user_arg;

    my_op = &op->entry[op->ndx - 1];
    op->ndx = my_op->prev_ndx;

    /* see comment in globus_l_xio_driver_open_op_kickout */
    if(op->canceled == (op->start_ndx + 1))
    {
        op->canceled = 0;
    }

    if(my_op->cb != NULL)
    {
        my_op->cb(
            op,
            GlobusXIOObjToResult(op->cached_obj),
            my_op->user_arg);
    }
    else
    {
        globus_xio_driver_finished_close(
            op, GlobusXIOObjToResult(op->cached_obj));
    }
    GlobusXIODebugInternalExit();
}

/*
 *  driver callback mickout
 *
 *  when in a register the finish function kicks this out as a oneshot
 */
void
globus_l_xio_driver_op_accept_kickout(
    void *                                  user_arg)
{
    globus_i_xio_op_t *                     op;
    globus_i_xio_op_entry_t *               my_op;
    GlobusXIOName(globus_l_xio_driver_op_accept_kickout);
                                                                                
    GlobusXIODebugInternalEnter();
    op = (globus_i_xio_op_t *) user_arg;
                                                                                
    my_op = &op->entry[op->ndx - 1];
    op->ndx = my_op->prev_ndx;
                                                                                
    /* see comment in globus_l_xio_driver_open_op_kickout */
    if(op->canceled == (op->start_ndx + 1))
    {
        op->canceled = 0;
    }

    if(my_op->cb != NULL)
    {
        my_op->cb(
            op,
            GlobusXIOObjToResult(op->cached_obj),
            my_op->user_arg);
    }
    else
    {
        globus_xio_driver_finished_accept(
            op, NULL, GlobusXIOObjToResult(op->cached_obj));
    }
    GlobusXIODebugInternalExit();
}


void
globus_l_xio_driver_open_op_kickout(
    void *                                  user_arg)
{
    globus_i_xio_handle_t *                 handle;
    globus_i_xio_context_t *                context;
    globus_i_xio_context_entry_t *          my_context;
    int                                     ndx = 0;
    int                                     wb_ndx;
    globus_i_xio_op_entry_t *               my_op;
    globus_i_xio_op_t *                     op;
    globus_xio_operation_type_t             deliver_type;
    GlobusXIOName(globus_l_xio_driver_open_op_kickout);
    
    GlobusXIODebugInternalEnter();

    op = (globus_i_xio_op_t *) user_arg;

    my_op = &op->entry[op->ndx - 1];
    op->ndx = my_op->prev_ndx;
    ndx = op->ndx;
    my_context = &op->_op_context->entry[ndx];
    handle = op->_op_handle;
    context = op->_op_context;

    deliver_type = my_op->type;
    my_op->deliver_type =&deliver_type;

    /* if index == the level at which is was canceld then we reset the 
       canceled flag for the operation.  This should be safe outside of
       the lock because once cancel is set it is not unset and we only
       provide a best effort cancel server, meaning that is you call cancel
       it is possible that you get your operation back successfully. */
    if(op->canceled == (op->start_ndx + 1))
    {
        op->canceled = 0;
    }

    if(ndx == 0)
    {
        /* at top level the callback should never be null */
        globus_assert(my_op->cb != NULL);
        globus_thread_blocking_space_callback_push(
            globus_i_xio_will_block_cb,
            (void *) op,
            op->blocking ? GLOBUS_CALLBACK_GLOBAL_SPACE: handle->space,
            &wb_ndx);
        my_op->cb(op, GlobusXIOObjToResult(op->cached_obj), my_op->user_arg);
        globus_thread_blocking_callback_pop(&wb_ndx);
    }
    else
    {
        if(my_op->cb == NULL)
        {
            globus_xio_driver_finished_open(NULL, NULL, op, 
                GlobusXIOObjToResult(op->cached_obj));
        }
        else
        {
            my_op->cb(op, 
                GlobusXIOObjToResult(op->cached_obj), my_op->user_arg);
        }
    }

    globus_xio_driver_open_delivered(op, ndx, &deliver_type);

    GlobusXIODebugInternalExit();
}

/**************************************************************************
 *                  context driver api funcitons
 *                  ----------------------------
 *************************************************************************/
globus_result_t
globus_xio_driver_handle_close(
    globus_xio_driver_handle_t              driver_handle)
{
    globus_i_xio_context_entry_t *          context_entry;
    globus_i_xio_context_t *                xio_context;
    globus_bool_t                           destroy_context = GLOBUS_FALSE;
    globus_result_t                         res = GLOBUS_SUCCESS;
    GlobusXIOName(globus_xio_driver_context_close);

    GlobusXIODebugInternalEnter();

    context_entry = driver_handle;
    xio_context = context_entry->whos_my_daddy;

    globus_mutex_lock(&xio_context->mutex);
    {
        if(context_entry->state != GLOBUS_XIO_CONTEXT_STATE_CLOSED)
        {
            res = GlobusXIOErrorInvalidState(context_entry->state);
        }
        else
        {
            xio_context->ref--;
            if(xio_context->ref == 0)
            {
                GlobusXIODebugPrintf(
                    GLOBUS_XIO_DEBUG_INFO,
               ("[globus_xio_driver_context_close] :: context->ref == 0.\n"));
                destroy_context = GLOBUS_TRUE;
            }
        }
    }
    globus_mutex_unlock(&xio_context->mutex);

    /* clean up the entry */
    if(destroy_context)
    {
        globus_i_xio_context_destroy(xio_context);
    }

    GlobusXIODebugInternalExit();

    return res;
}

void
globus_i_xio_context_destroy(
    globus_i_xio_context_t *                xio_context)
{
    GlobusXIOName(globus_i_xio_context_destroy);

    GlobusXIODebugInternalEnter();
    globus_assert(xio_context->ref == 0);

    GlobusXIODebugPrintf(
        GLOBUS_XIO_DEBUG_INFO_VERBOSE, 
        ("  context @ 0x%x: ref=%d size=%d\n", 
            xio_context, xio_context->ref, xio_context->stack_size));

    globus_mutex_destroy(&xio_context->mutex);
    globus_memory_destroy(&xio_context->op_memory);
    globus_free(xio_context);

    GlobusXIODebugInternalExit();
}

globus_i_xio_context_t *
globus_i_xio_context_create(
    globus_i_xio_target_t *                 xio_target)
{
    globus_i_xio_context_t *                xio_context;
    int                                     size;
    int                                     ctr;
    GlobusXIOName(globus_i_xio_context_create);

    GlobusXIODebugInternalEnter();

    size = sizeof(globus_i_xio_context_t) +
        (sizeof(globus_i_xio_context_entry_t) * (xio_target->stack_size - 1));

    xio_context = (globus_i_xio_context_t *) globus_malloc(size);
    if(xio_context != NULL)
    {
        memset(xio_context, '\0', size);

        globus_mutex_init(&xio_context->mutex, NULL);
        xio_context->stack_size = xio_target->stack_size;
        globus_memory_init(&xio_context->op_memory,
            sizeof(globus_i_xio_op_t) +
                (sizeof(globus_i_xio_op_entry_t) *
                    (xio_target->stack_size - 1)),
            GLOBUS_XIO_HANDLE_DEFAULT_OPERATION_COUNT);
        xio_context->ref++;
        for(ctr = 0; ctr < xio_context->stack_size; ctr++)
        {
            xio_context->entry[ctr].whos_my_daddy = xio_context;
        }
    }

    GlobusXIODebugInternalExit();

    return xio_context;
}

/**************************************************************************
 *                  macro wrapper functions
 *                  -----------------------
 *
 *  this is mainly a compile test, but who knows, someone may want it
 *************************************************************************/
void *
globus_i_xio_attr_get_ds(
    globus_i_xio_attr_t *                   attr,
    globus_xio_driver_t                     driver)
{
    void *                                  rc;

    GlobusIXIOAttrGetDS(rc, attr, driver);

    return rc;
}

/*
 *  read ahead stuff
 */
void 
globus_xio_driver_operation_destroy(
    globus_xio_operation_t                  operation)
{
    globus_i_xio_context_t *                l_context;
    globus_bool_t                           destroy_context = GLOBUS_FALSE;
    globus_i_xio_op_t *                     op;
    GlobusXIOName(globus_xio_driver_operation_destroy);

    GlobusXIODebugInternalEnter();

    op = operation;
    l_context = op->_op_context;

    globus_mutex_lock(&l_context->mutex);
    {
        GlobusXIOOpDec(op);
        if(op->ref == 0)
        {
            l_context->ref--;
            if(l_context->ref == 0)
            {
                GlobusXIODebugPrintf(
                    GLOBUS_XIO_DEBUG_INFO,
      ("[globus_xio_driver_operation_destroy] :: context->ref == 0.\n"));
                destroy_context = GLOBUS_TRUE;
            }
            globus_memory_push_node(&l_context->op_memory, op);
        }
    }
    globus_mutex_unlock(&l_context->mutex);

    if(destroy_context)
    {
        globus_i_xio_context_destroy(l_context);
    }
    GlobusXIODebugInternalExit();
}

globus_result_t
globus_xio_driver_operation_create(
    globus_xio_operation_t *                operation,
    globus_xio_driver_handle_t              driver_handle)
{
    globus_i_xio_op_t *                     op;
    globus_i_xio_op_entry_t *               my_op;
    globus_result_t                         res;
    globus_i_xio_context_entry_t *          my_context;
    globus_i_xio_context_t *                l_context;
    int                                     index = -1;
    int                                     ctr;
    globus_bool_t                           done = GLOBUS_FALSE;
    GlobusXIOName(globus_xio_driver_operation_create);

    GlobusXIODebugEnter();

    my_context = driver_handle;
    l_context = my_context->whos_my_daddy;
    for(ctr = 0; ctr < l_context->stack_size && !done; ctr++)
    {
        if(my_context == &l_context->entry[ctr])
        {
            index = ctr;
            done = GLOBUS_TRUE;
        }
    }

    if(index == -1)
    {
        res = GlobusXIOErrorParameter("context");
        goto err;
    }

    GlobusXIOOperationCreate(op, l_context);
    if(op == NULL)
    {
        res = GlobusXIOErrorMemory("op");
        goto err;
    }
    op->ndx = index + 1;
    op->start_ndx = index;

    op->type = GLOBUS_XIO_OPERATION_TYPE_DRIVER;
    op->state = GLOBUS_XIO_OP_STATE_OPERATING;
    op->ref = 1;
    op->_op_handle = NULL;
    op->_op_context = l_context;
    op->_op_handle_timeout_cb = NULL;

    my_op = &op->entry[index];
    my_op->_op_ent_nbytes = 0;
    my_op->_op_ent_wait_for = 0;
    my_op->prev_ndx = -1;
    my_op->type = GLOBUS_XIO_OPERATION_TYPE_DRIVER;

    l_context->ref++;

    *operation = op;

    GlobusXIODebugExit();
    return GLOBUS_SUCCESS;

  err:

    GlobusXIODebugExitWithError();
    return res;
}

globus_result_t
globus_i_xio_driver_attr_cntl(
    globus_i_xio_attr_t *                   attr,
    globus_xio_driver_t                     driver,
    int                                     cmd,
    va_list                                 ap)
{
    globus_result_t                         res;
    void *                                  ds;
    globus_xio_attr_cmd_t                   general_cmd;
    globus_xio_timeout_server_callback_t    server_timeout_cb;
    globus_xio_timeout_callback_t           timeout_cb;
    globus_reltime_t *                      delay_time;
    globus_callback_space_t                 space;
    GlobusXIOName(globus_i_xio_driver_attr_cntl);

    GlobusXIODebugEnter();

    if(driver != NULL)
    {
        GlobusIXIOAttrGetDS(ds, attr, driver);
        if(ds == NULL)
        {
            res = driver->attr_init_func(&ds);
            if(res != GLOBUS_SUCCESS)
            {
                goto err;
            }
            if(attr->ndx >= attr->max)
            {
                attr->max *= 2;
                attr->entry = (globus_i_xio_attr_ent_t *)
                    globus_realloc(attr->entry, attr->max *
                            sizeof(globus_i_xio_attr_ent_t));
            }
            attr->entry[attr->ndx].driver = driver;
            attr->entry[attr->ndx].driver_data = ds;
            attr->ndx++;
        }
        res = driver->attr_cntl_func(ds, cmd, ap);
        if(res != GLOBUS_SUCCESS)
        {
            goto err;
        }
    }
    else
    {
        general_cmd = cmd;

        switch(general_cmd)
        {
            case GLOBUS_XIO_ATTR_SET_TIMEOUT_ALL:
                timeout_cb = va_arg(ap, globus_xio_timeout_callback_t);
                delay_time = va_arg(ap, globus_reltime_t *);

                attr->open_timeout_cb = timeout_cb;
                attr->close_timeout_cb = timeout_cb;
                attr->read_timeout_cb = timeout_cb;
                attr->write_timeout_cb = timeout_cb;

                GlobusTimeReltimeCopy(attr->open_timeout_period, *delay_time);
                GlobusTimeReltimeCopy(attr->close_timeout_period, *delay_time);
                GlobusTimeReltimeCopy(attr->read_timeout_period, *delay_time);
                GlobusTimeReltimeCopy(attr->write_timeout_period, *delay_time);
                break;

            case GLOBUS_XIO_ATTR_SET_TIMEOUT_OPEN:
                timeout_cb = va_arg(ap, globus_xio_timeout_callback_t);
                delay_time = va_arg(ap, globus_reltime_t *);
                attr->open_timeout_cb = timeout_cb;
                GlobusTimeReltimeCopy(attr->open_timeout_period, *delay_time);
                break;

            case GLOBUS_XIO_ATTR_SET_TIMEOUT_CLOSE:
                timeout_cb = va_arg(ap, globus_xio_timeout_callback_t);
                delay_time = va_arg(ap, globus_reltime_t *);

                attr->close_timeout_cb = timeout_cb;
                GlobusTimeReltimeCopy(attr->close_timeout_period, *delay_time);
                break;

            case GLOBUS_XIO_ATTR_SET_TIMEOUT_READ:
                timeout_cb = va_arg(ap, globus_xio_timeout_callback_t);
                delay_time = va_arg(ap, globus_reltime_t *);

                attr->read_timeout_cb = timeout_cb;
                GlobusTimeReltimeCopy(attr->read_timeout_period, *delay_time);
                break;

            case GLOBUS_XIO_ATTR_SET_TIMEOUT_WRITE:
                timeout_cb = va_arg(ap, globus_xio_timeout_callback_t);
                delay_time = va_arg(ap, globus_reltime_t *);

                attr->write_timeout_cb = timeout_cb;
                GlobusTimeReltimeCopy(attr->write_timeout_period, *delay_time);
                break;

            case GLOBUS_XIO_ATTR_SET_TIMEOUT_ACCEPT:
                server_timeout_cb =
                    va_arg(ap, globus_xio_timeout_server_callback_t);
                delay_time = va_arg(ap, globus_reltime_t *);

                attr->accept_timeout_cb = server_timeout_cb;
                GlobusTimeReltimeCopy(attr->accept_timeout_period, *delay_time);
                break;
            case GLOBUS_XIO_ATTR_SET_SPACE:
                space = va_arg(ap, globus_callback_space_t);
                res = globus_callback_space_reference(space);
                if(res != GLOBUS_SUCCESS)
                {
                    goto err;
                }
                globus_callback_space_destroy(attr->space);
                attr->space = space;
                break;
        }

        res = GLOBUS_SUCCESS;
    }

    GlobusXIODebugExit();
    return GLOBUS_SUCCESS;

  err:
    GlobusXIODebugExitWithError();
    return res;
}

globus_result_t
globus_i_xio_driver_dd_cntl(
    globus_i_xio_op_t *                     op,
    globus_xio_driver_t                     driver,
    int                                     cmd,
    va_list                                 ap)
{
    globus_result_t                         res;
    int                                     ndx;
    int                                     ctr;
    GlobusXIOName(globus_i_xio_driver_dd_cntl);

    GlobusXIODebugEnter();

    if(driver != NULL)
    {
        ndx = -1;
        for(ctr = 0; ctr < op->stack_size && ndx == -1; ctr++)
        {
            if(driver == op->_op_context->entry[ctr].driver)
            {
                if(op->entry[ctr].dd == NULL)
                {
                    res = op->_op_context->entry[ctr].driver->attr_init_func(
                            &op->entry[ctr].dd);
                    if(res != GLOBUS_SUCCESS)
                    {
                        goto err;
                    }
                }
                ndx = ctr;
            }
        }
        if(ndx == -1)
        {
            res = GlobusXIOErrorInvalidDriver("not found");
            goto err;
        }

        if(op->_op_context->entry[ndx].driver->attr_cntl_func)
        {
            res = op->_op_context->entry[ndx].driver->attr_cntl_func(
                    op->entry[ndx].dd,
                    cmd,
                    ap);
            if(res != GLOBUS_SUCCESS)
            {
                goto err;
            }
        }
        else
        {
            res = GlobusXIOErrorInvalidDriver("driver doesn't support dd cntl");
            goto err;
        }
    }
    else
    {
    }
    GlobusXIODebugExit();

    return GLOBUS_SUCCESS;
  err:

    GlobusXIODebugExitWithError();
    return res;
}

globus_result_t
globus_xio_driver_attr_cntl(
    globus_xio_operation_t                  op,
    globus_xio_driver_t                     driver,
    int                                     cmd,
    ...)
{
    globus_result_t                         res;
    va_list                                 ap;
    GlobusXIOName(globus_xio_driver_data_descriptor_cntl);

    GlobusXIODebugEnter();

    if(op == NULL)
    {
        res = GlobusXIOErrorParameter("op");
        goto err;
    }

#   ifdef HAVE_STDARG_H
    {
        va_start(ap, cmd);
    }
#   else
    {
        va_start(ap);
    }
#   endif

    res = globus_i_xio_driver_dd_cntl(op, driver, cmd, ap);

    va_end(ap);

    if(res != GLOBUS_SUCCESS)
    {
        goto err;
    }

    GlobusXIODebugExit();

    return GLOBUS_SUCCESS;

  err:

    GlobusXIODebugExitWithError();
    return res;
}

globus_result_t
globus_xio_driver_data_descriptor_cntl(
    globus_xio_operation_t                  op,
    globus_xio_driver_t                     driver,
    int                                     cmd,
    ...)
{
    globus_result_t                         res;
    va_list                                 ap;
    GlobusXIOName(globus_xio_driver_data_descriptor_cntl);

    GlobusXIODebugEnter();

    if(op == NULL)
    {
        res = GlobusXIOErrorParameter("op");
        goto err;
    }

#   ifdef HAVE_STDARG_H
    {
        va_start(ap, cmd);
    }
#   else
    {
        va_start(ap);
    }
#   endif

    res = globus_i_xio_driver_dd_cntl(op, driver, cmd, ap);

    va_end(ap);

    if(res != GLOBUS_SUCCESS)
    {
        goto err;
    }

    GlobusXIODebugExit();

    return GLOBUS_SUCCESS;

  err:

    GlobusXIODebugExitWithError();
    return res;
}

globus_result_t
globus_i_xio_driver_handle_cntl(
    globus_i_xio_context_t *                context,
    globus_xio_driver_t                     driver,
    int                                     cmd,
    va_list                                 ap)
{
    globus_result_t                         res;
    int                                     ctr;
    int                                     ndx;
    GlobusXIOName(globus_i_xio_driver_handle_cntl);

    GlobusXIODebugEnter();

    if(context == NULL)
    {
        res = GlobusXIOErrorParameter("conext");
        goto err;
    }

    if(driver != NULL)
    {
        ndx = -1;
        for(ctr = 0; ctr < context->stack_size && ndx == -1; ctr++)
        {
            if(driver == context->entry[ctr].driver)
            {
                res = context->entry[ctr].driver->handle_cntl_func(
                        context->entry[ctr].driver_handle,
                        cmd,
                        ap);
                if(res != GLOBUS_SUCCESS)
                {
                    goto err;
                }
                ndx = ctr;
            }
        }
        if(ndx == -1)
        {
            /* throw error */
            res = GlobusXIOErrorInvalidDriver("not found");
            goto err;
        }
    }
    else
    {
        /* do general settings */
    }

    GlobusXIODebugExit();
    return GLOBUS_SUCCESS;

  err:

    GlobusXIODebugExitWithError();
    return res;
}

globus_result_t
globus_xio_driver_handle_cntl(
    globus_xio_driver_handle_t              driver_handle,
    globus_xio_driver_t                     driver,
    int                                     cmd,
    ...)
{
    globus_result_t                         res;
    va_list                                 ap;
    globus_i_xio_context_t *                context;
    GlobusXIOName(globus_xio_driver_handle_cntl);

    GlobusXIODebugEnter();

    if(driver_handle == NULL)
    {
        res = GlobusXIOErrorParameter("driver_handle");
        goto err;
    }
    context = driver_handle->whos_my_daddy;
    if(context == NULL)
    {
        res = GlobusXIOErrorParameter("op");
        goto err;
    }
#   ifdef HAVE_STDARG_H
    {
        va_start(ap, cmd);
    }
#   else
    {
        va_start(ap);
    }
#   endif

    res = globus_i_xio_driver_handle_cntl(context, driver, cmd, ap);
    va_end(ap);
    if(res != GLOBUS_SUCCESS)
    {
        goto err;
    }

    GlobusXIODebugExit();
    return GLOBUS_SUCCESS;

  err:

    GlobusXIODebugExitWithError();
    return res;
}

globus_result_t
globus_xio_driver_operation_cancel(
    globus_xio_operation_t                  operation)
{
    globus_result_t                         res;
    globus_i_xio_handle_t *                 handle;
    globus_i_xio_op_t *                     op;
    GlobusXIOName(globus_xio_driver_operation_cancel);

    GlobusXIODebugEnter();

    op = (globus_i_xio_op_t *) operation;
    if(op == NULL)
    {
        res = GlobusXIOErrorParameter("op");
        goto err;
    }

    handle = op->_op_handle;

    globus_mutex_lock(&handle->cancel_mutex);
    {
        res = globus_i_xio_operation_cancel(op);
    }
    globus_mutex_unlock(&handle->cancel_mutex);

    GlobusXIODebugExit();
    return GLOBUS_SUCCESS;

  err:

    GlobusXIODebugExitWithError();
    return res;
}

/***************************************************************************
 *                      driver setup functions
 *                      ----------------------
 **************************************************************************/
globus_result_t
globus_xio_driver_init(
    globus_xio_driver_t *                   out_driver,
    const char *                            driver_name,
    void *                                  user_data)
{
    globus_i_xio_driver_t *                 driver;
    globus_result_t                         res;
    GlobusXIOName(globus_xio_driver_init);

    GlobusXIODebugEnter();

    driver = (globus_i_xio_driver_t *)
            globus_malloc(sizeof(globus_i_xio_driver_t));
    if(driver == NULL)
    {
        res = GlobusXIOErrorMemory("driver");
        goto err;
    }
    memset(driver, '\0', sizeof(globus_i_xio_driver_t));
    
    driver->name = globus_libc_strdup(driver_name);
    if(!driver->name)
    {
        globus_free(driver);
        res = GlobusXIOErrorMemory("driver->name");
        goto err;
    }
    
    driver->user_data = user_data;

    *out_driver = driver;

    GlobusXIODebugExit();
    return GLOBUS_SUCCESS;

  err:

    GlobusXIODebugExitWithError();
    return res;
}

globus_result_t
globus_xio_driver_get_user_data(
    globus_xio_driver_t                     in_driver,
    void **                                 out_user_data)
{
    globus_result_t                         res;
    globus_i_xio_driver_t *                 driver;
    GlobusXIOName(globus_xio_driver_get_user_data);

    GlobusXIODebugEnter();
    if(in_driver == NULL)
    {
        res = GlobusXIOErrorMemory("in_driver");
        goto err;
    }
    if(out_user_data == NULL)
    {
        res = GlobusXIOErrorMemory("out_user_data");
        goto err;
    }

    driver = in_driver;

    *out_user_data = driver->user_data;

    GlobusXIODebugExit();
    return GLOBUS_SUCCESS;

  err:

    GlobusXIODebugExitWithError();
    return res;
}

globus_result_t
globus_xio_driver_destroy(
    globus_xio_driver_t                     driver)
{
    GlobusXIOName(globus_xio_driver_destroy);

    GlobusXIODebugEnter();
    globus_free(driver->name);
    globus_free(driver);
    GlobusXIODebugExit();

    return GLOBUS_SUCCESS;
}

globus_result_t
globus_xio_driver_set_transport(
    globus_xio_driver_t                     driver,
    globus_xio_driver_transport_open_t      transport_open_func,
    globus_xio_driver_close_t               close_func,
    globus_xio_driver_read_t                read_func,
    globus_xio_driver_write_t               write_func,
    globus_xio_driver_handle_cntl_t         handle_cntl_func)
{
    GlobusXIOName(globus_xio_driver_set_transport);

    GlobusXIODebugEnter();
    driver->transport_open_func = transport_open_func;
    driver->close_func = close_func;
    driver->read_func = read_func;
    driver->write_func = write_func;
    driver->handle_cntl_func = handle_cntl_func;
    GlobusXIODebugExit();

    return GLOBUS_SUCCESS;
}

globus_result_t
globus_xio_driver_set_transform(
    globus_xio_driver_t                     driver,
    globus_xio_driver_transform_open_t      transform_open_func,
    globus_xio_driver_close_t               close_func,
    globus_xio_driver_read_t                read_func,
    globus_xio_driver_write_t               write_func,
    globus_xio_driver_handle_cntl_t         handle_cntl_func,
    globus_xio_driver_push_driver_t         push_driver_func)
{
    GlobusXIOName(globus_xio_driver_set_transform);

    GlobusXIODebugEnter();
    driver->transform_open_func = transform_open_func;
    driver->close_func = close_func;
    driver->read_func = read_func;
    driver->write_func = write_func;
    driver->handle_cntl_func = handle_cntl_func;
    driver->push_driver_func = push_driver_func;
    GlobusXIODebugExit();

    return GLOBUS_SUCCESS;
}

globus_result_t
globus_xio_driver_set_client(
    globus_xio_driver_t                     driver,
    globus_xio_driver_target_init_t         target_init_func,
    globus_xio_driver_target_cntl_t         target_cntl_func,
    globus_xio_driver_target_destroy_t      target_destroy_func)
{
    GlobusXIOName(globus_xio_driver_set_client);

    GlobusXIODebugEnter();
    driver->target_init_func = target_init_func;
    driver->target_cntl_func = target_cntl_func;
    driver->target_destroy_func = target_destroy_func;
    GlobusXIODebugExit();

    return GLOBUS_SUCCESS;
}

globus_result_t
globus_xio_driver_set_server(
    globus_xio_driver_t                     driver,
    globus_xio_driver_server_init_t         server_init_func,
    globus_xio_driver_server_accept_t       server_accept_func,
    globus_xio_driver_server_destroy_t      server_destroy_func,
    globus_xio_driver_server_cntl_t         server_cntl_func,
    globus_xio_driver_target_destroy_t      target_destroy_func)
{
    GlobusXIOName(globus_xio_driver_set_server);

    GlobusXIODebugEnter();
    driver->server_init_func = server_init_func;
    driver->server_accept_func = server_accept_func;
    driver->server_destroy_func = server_destroy_func;
    driver->server_cntl_func = server_cntl_func;
    driver->target_destroy_func = target_destroy_func;
    GlobusXIODebugExit();

    return GLOBUS_SUCCESS;
}

globus_result_t
globus_xio_driver_set_attr(
    globus_xio_driver_t                     driver,
    globus_xio_driver_attr_init_t           attr_init_func,
    globus_xio_driver_attr_copy_t           attr_copy_func,
    globus_xio_driver_attr_cntl_t           attr_cntl_func,
    globus_xio_driver_attr_destroy_t        attr_destroy_func)
{
    GlobusXIOName(globus_xio_driver_set_attr);

    GlobusXIODebugEnter();

    if(driver == NULL)
    {
        return GlobusXIOErrorParameter("driver");
    }
    if(attr_init_func == NULL)
    {
        return GlobusXIOErrorParameter("attr_init_func");
    }
    if(attr_copy_func == NULL)
    {
        return GlobusXIOErrorParameter("attr_copy_func");
    }
    if(attr_destroy_func == NULL)
    {
        return GlobusXIOErrorParameter("attr_destroy_func");
    }

    driver->attr_init_func = attr_init_func;
    driver->attr_copy_func = attr_copy_func;
    driver->attr_cntl_func = attr_cntl_func;
    driver->attr_destroy_func = attr_destroy_func;

    GlobusXIODebugExit();

    return GLOBUS_SUCCESS;
}

