/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** GUIX Component                                                        */
/**                                                                       */
/**   Multi Line Text View Management (Multi Line Text View)              */
/**                                                                       */
/**************************************************************************/

#define GX_SOURCE_CODE


/* Include necessary system files.  */

#include "gx_api.h"
#include "gx_multi_line_text_view.h"
#include "gx_window.h"
#include "gx_system.h"
#include "gx_utility.h"

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _gx_multi_line_text_view_event_process              PORTABLE C      */
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Kenneth Maxwell, Microsoft Corporation                              */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This service processes an event for a multi-line text view widget.  */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    text_view                             Multi-line text view widget   */
/*                                            control block               */
/*    event_ptr                             Point to event to process     */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    status                                Completion status             */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _gx_multi_line_text_view_text_id_set  Set new text id               */
/*    _gx_multi_line_text_view_scroll       Scroll the text view window   */
/*    _gx_window_event_process              Window event process          */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application Code                                                    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Kenneth Maxwell          Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/

UINT _gx_multi_line_text_view_event_process(GX_MULTI_LINE_TEXT_VIEW *text_view, GX_EVENT *event_ptr)
{
    switch (event_ptr -> gx_event_type)
    {
    case GX_EVENT_SHOW:
        _gx_window_event_process((GX_WINDOW *)text_view, event_ptr);

        /* If the text view text ID is specified.*/
        if (text_view -> gx_multi_line_text_view_text_id)
        {
            _gx_multi_line_text_view_text_id_set(text_view, text_view -> gx_multi_line_text_view_text_id);
        }

        /* Get visible rows. */
        _gx_multi_line_text_view_visible_rows_compute(text_view);
        break;

    case GX_EVENT_RESIZED:
        if (text_view -> gx_widget_status & GX_STATUS_VISIBLE)
        {
            text_view -> gx_multi_line_text_view_line_index_old = GX_TRUE;
        }
        break;

    case GX_EVENT_CLIENT_UPDATED:
        if (text_view -> gx_widget_status & GX_STATUS_VISIBLE)
        {
            text_view -> gx_multi_line_text_view_line_index_old = GX_TRUE;
            _gx_system_dirty_mark((GX_WIDGET *)text_view);
        }
        break;

    case GX_EVENT_STYLE_CHANGED:
        if (text_view -> gx_widget_status & GX_STATUS_VISIBLE)
        {
            if ((event_ptr -> gx_event_payload.gx_event_ulongdata & GX_STYLE_BORDER_MASK) !=
                (text_view -> gx_widget_style & GX_STYLE_BORDER_MASK))
            {
                text_view -> gx_multi_line_text_view_line_index_old = GX_TRUE;
            }
        }
        break;

    case GX_EVENT_LANGUAGE_CHANGE:
        if (text_view -> gx_multi_line_text_view_text_id)
        {
            _gx_multi_line_text_view_text_id_set(text_view, text_view -> gx_multi_line_text_view_text_id);
        }
        break;

    case GX_EVENT_VERTICAL_SCROLL:
        _gx_multi_line_text_view_scroll(text_view, (GX_VALUE)(event_ptr->gx_event_payload.gx_event_intdata[1] - event_ptr->gx_event_payload.gx_event_intdata[0]));
        return 0;
    }

    return(_gx_window_event_process((GX_WINDOW *)text_view, event_ptr));
}

