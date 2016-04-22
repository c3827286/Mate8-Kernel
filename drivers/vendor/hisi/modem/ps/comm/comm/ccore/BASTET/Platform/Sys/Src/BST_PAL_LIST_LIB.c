/******************************************************************************

                  版权所有 (C), 2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : BST_PAL_LIST_LIB.c
  版 本 号   : 初稿
  作    者   :
  生成日期   : 2014年07月14日
  最近修改   :
  功能描述   : 实现LIST链表相关的操作
  函数列表   :
  修改历史   :
  1.日    期   : 2014年07月14日
    作    者   :
    修改内容   : 建立文件

******************************************************************************/

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "BST_PAL_LIST.h"
#include "BST_OS_Memory.h"
#include "stdlib.h"
#if (VOS_RTOSCK == VOS_OS_VER)
#define    THIS_FILE_ID        PS_FILE_ID_BST_PAL_LIST_LIB_C

/******************************************************************************
   2 宏定义
******************************************************************************/

/*****************************************************************************
  3 函数声明
*****************************************************************************/

/******************************************************************************
   4 私有定义
******************************************************************************/

/******************************************************************************
   5 全局变量定义
******************************************************************************/

/******************************************************************************
   6 函数实现
******************************************************************************/

/*****************************************************************************
 函 数 名  : BST_ListInit
 功能描述  : LIST初始化函数
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2015年07月14日
      作    者   :
      修改内容   : 新生成函数
*****************************************************************************/
BST_VOID BST_ListInit( LIST *pList )
{
    if( BST_NULL_PTR == pList )
    {
        return;
    }

    pList->node.next     = BST_NULL_PTR;
    pList->node.previous = BST_NULL_PTR;
    pList->count         = 0;

    return;
}

/*****************************************************************************
 函 数 名  : BST_ListAdd
 功能描述  : LIST添加函数
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2015年07月14日
      作    者   :
      修改内容   : 新生成函数
*****************************************************************************/
BST_VOID BST_ListAdd( LIST *pList, NODE *pNode )
{
    NODE                               *pNext;
    NODE                               *pPrev;

    if( ( BST_NULL_PTR == pList ) || ( BST_NULL_PTR == pNode ) )
    {
        return;
    }

    pPrev = pList->node.previous;

    if (pPrev == BST_NULL_PTR)
    {
        pNext            = pList->node.next;
        pList->node.next = pNode;
    }
    else
    {
        pNext = pPrev->next;
        pPrev->next = pNode;
    }

    if (pNext == BST_NULL_PTR)
    {
        pList->node.previous = pNode;
    }
    else
    {
        pNext->previous = pNode;
    }

    pNode->next = pNext;
    pNode->previous = pPrev;

    pList->count++;

    return;
}

/*****************************************************************************
 函 数 名  : BST_ListCount
 功能描述  : 返回当前LIST链表中的个数
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2015年07月14日
      作    者   :
      修改内容   : 新生成函数
*****************************************************************************/
BST_INT16 BST_ListCount( LIST *pList )
{
    if( BST_NULL_PTR == pList )
    {
        return 0;
    }

    return (pList->count);
}

/*****************************************************************************
 函 数 名  : BST_ListDelete
 功能描述  : LIST删除操作，删除LIST中某个节点
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2015年07月14日
      作    者   :
      修改内容   : 新生成函数
*****************************************************************************/
BST_VOID BST_ListDelete( LIST *pList, NODE *pNode )
{
    if( ( BST_NULL_PTR == pList ) || ( BST_NULL_PTR == pNode ) )
    {
        return;
    }

    if (pNode->previous == BST_NULL_PTR)
    {
        pList->node.next = pNode->next;
    }
    else
    {
        pNode->previous->next = pNode->next;
    }

    if (pNode->next == BST_NULL_PTR)
    {
        pList->node.previous = pNode->previous;
    }
    else
    {
        pNode->next->previous = pNode->previous;
    }

    if( pList->count > 0 )
    {
        pList->count--;
    }

    return;
}

/*****************************************************************************
 函 数 名  : BST_ListFirst
 功能描述  : 返回LIST链表中第一个节点
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2015年07月14日
      作    者   :
      修改内容   : 新生成函数
*****************************************************************************/
NODE *BST_ListFirst ( LIST *pList )
{
    if( BST_NULL_PTR == pList )
    {
        return BST_NULL_PTR;
    }

    return (pList->node.next);
}

/*****************************************************************************
 函 数 名  : BST_ListGet
 功能描述  : 返回LIST的一个节点
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2015年07月14日
      作    者   :
      修改内容   : 新生成函数
*****************************************************************************/
NODE *BST_ListGet( LIST *pList )
{
    NODE                               *pNode;

    if( BST_NULL_PTR == pList )
    {
        return BST_NULL_PTR;
    }

    pNode = pList->node.next;

    if (pNode != BST_NULL_PTR)
    {
        pList->node.next = pNode->next;

        if (pNode->next == BST_NULL_PTR)
        {
            pList->node.previous = BST_NULL_PTR;
        }
        else
        {
            pNode->next->previous = BST_NULL_PTR;
        }

        if( pList->count > 0 )
        {
            pList->count--;
        }
    }

    return (pNode);
}

/*****************************************************************************
 函 数 名  : BST_ListNext
 功能描述  : 返回某特定节点的下一个节点
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2015年07月14日
      作    者   :
      修改内容   : 新生成函数
*****************************************************************************/
NODE *BST_ListNext( NODE *pNode )
{
    if( BST_NULL_PTR == pNode )
    {
        return BST_NULL_PTR;
    }

    return (pNode->next);
}

/*****************************************************************************
 函 数 名  : BST_ListFree
 功能描述  : 链表清空函数
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2015年07月14日
      作    者   :
      修改内容   : 新生成函数
*****************************************************************************/
BST_VOID BST_ListFree( LIST *pList )
{
    NODE                               *p1;
    NODE                               *p2;

    if( BST_NULL_PTR == pList )
    {
        return;
    }

    if (pList->count > 0)
    {
        p1 = pList->node.next;

        while (p1 != BST_NULL_PTR)
        {
            p2 = p1->next;
            BST_OS_FREE( p1 );
            p1 = p2;
        }

        pList->count         = 0;
        pList->node.next     = BST_NULL_PTR;
        pList->node.previous = BST_NULL_PTR;
    }

    return;
}
#endif