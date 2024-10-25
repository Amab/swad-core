// swad_tree.c: course tree

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2024 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*****************************************************************************/
/********************************* Headers ***********************************/
/*****************************************************************************/

#define _GNU_SOURCE 		// For asprintf
#include <linux/limits.h>	// For PATH_MAX
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For calloc
#include <string.h>		// For string functions

#include "swad_action_list.h"
#include "swad_autolink.h"
#include "swad_box.h"
#include "swad_contracted_expanded.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_hidden_visible.h"
#include "swad_hierarchy_type.h"
#include "swad_HTML.h"
#include "swad_pagination.h"
#include "swad_parameter.h"
#include "swad_parameter_code.h"
#include "swad_photo.h"
#include "swad_program.h"
#include "swad_program_database.h"
#include "swad_program_resource.h"
#include "swad_role.h"
#include "swad_setting.h"
#include "swad_string.h"
#include "swad_tree.h"
#include "swad_tree_database.h"
#include "swad_view.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/******************************* Private types *******************************/
/*****************************************************************************/

#define Tre_NUM_MOVEMENTS_UP_DOWN 2
typedef enum
  {
   Tre_MOVE_UP,
   Tre_MOVE_DOWN,
  } Tre_MoveUpDown_t;

struct Level
  {
   unsigned Number;	// Numbers for each level from 1 to maximum level
   ConExp_ContractedOrExpanded_t ContractedOrExpanded;	// If each level from 1 to maximum level is expanded
   HidVis_HiddenOrVisible_t HiddenOrVisible;	// If each level...
						// ...from 1 to maximum level...
						// ...is hidden or visible
  };

/*****************************************************************************/
/***************************** Private variables *****************************/
/*****************************************************************************/

static struct
  {
   struct
     {
      bool IsRead;			// Is the list already read from database...
					// ...or it needs to be read?
      unsigned NumNodes;		// Number of nodes
      struct Tre_NodeHierarchy *Nodes;	// List of nodes
     } List;
   unsigned MaxLevel;		// Maximum level of nodes
   struct Level *Levels;	// Numbers and hidden for each level from 1 to maximum level
  } Tre_Gbl =
  {
   .List =
     {
      .IsRead     = false,
      .NumNodes   = 0,
      .Nodes      = NULL,
     },
   .MaxLevel      = 0,
   .Levels        = NULL
  };

static const char *Tre_NODE_SECTION_ID = "node_section";

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Tre_PutIconsListNodes (__attribute__((unused)) void *Args);
static void Tre_PutIconsEditNodes (__attribute__((unused)) void *Args);
static void Tre_PutIconToEditTree (void);
static void Tre_PutIconToViewTree (void);
static void Tre_PutIconToCreateNewNode (void);

static void Tre_WriteRowNode (Tre_TreeType_t TreeType,Tre_ListingType_t ListingType,
                              unsigned NumNode,struct Tre_Node *Node,
                              ConExp_ContractedOrExpanded_t ContractedOrExpanded,
                              long SelectedNodCod,
                              long SelectedRscCod);
static void Tre_PutIconToContractOrExpandNode (struct Tre_Node *Node,
                                               ConExp_ContractedOrExpanded_t ContractedOrExpanded,
                                               Vie_ViewType_t ViewType);
static void Tre_WriteNodeText (Tre_TreeType_t TreeType,
			       long NodCod,HidVis_HiddenOrVisible_t HiddenOrVisible);
static void Tre_WriteRowToCreateNode (Tre_TreeType_t TreeType,
				      long ParentNodCod,unsigned FormLevel);
static void Tre_SetTitleClass (char **TitleClass,unsigned Level);
static void Tre_FreeTitleClass (char *TitleClass);

static void Tre_SetMaxNodeLevel (unsigned Level);
static unsigned Tre_GetMaxNodeLevel (void);
static unsigned Tre_CalculateMaxNodeLevel (void);
static void Tre_CreateLevels (void);
static void Tre_FreeLevels (void);
static void Tre_IncreaseNumberInLevel (unsigned Level);
static unsigned Tre_GetCurrentNumberInLevel (unsigned Level);
static void Tre_WriteNumNode (unsigned Level);
static void Tre_WriteNumNewNode (unsigned Level);

static void Tre_SetExpandedLevel (unsigned Level,ConExp_ContractedOrExpanded_t ContractedOrExpanded);
static void Tre_SetHiddenLevel (unsigned Level,HidVis_HiddenOrVisible_t HiddenOrVisible);
static ConExp_ContractedOrExpanded_t Tre_GetExpandedLevel (unsigned Level);
static HidVis_HiddenOrVisible_t Tre_GetHiddenLevel (unsigned Level);

static bool Tre_CheckIfAllHigherLevelsAreExpanded (unsigned CurrentLevel);
static HidVis_HiddenOrVisible_t Tre_CheckIfAnyHigherLevelIsHidden (unsigned CurrentLevel);

static void Tre_PutFormsToRemEditOneNode (Tre_ListingType_t ListingType,
                                          unsigned NumNode,
                                          struct Tre_Node *Node,
                                          bool HighlightNode);
static bool Tre_CheckIfMoveUpIsAllowed (unsigned NumNode);
static bool Tre_CheckIfMoveDownIsAllowed (unsigned NumNode);
static bool Tre_CheckIfMoveLeftIsAllowed (unsigned NumNode);
static bool Tre_CheckIfMoveRightIsAllowed (unsigned NumNode);

static void Tre_GetNodeDataByCod (struct Tre_Node *Node);
static void Tre_GetNodeDataFromRow (MYSQL_RES **mysql_res,
                                    struct Tre_Node *Node,
                                    unsigned NumRows);

static void Tre_HideOrUnhideNode (HidVis_HiddenOrVisible_t HiddenOrVisible);

static void Tre_MoveUpDownNode (Tre_MoveUpDown_t UpDown);
static bool Tre_ExchangeNodeRanges (Tre_TreeType_t TreeType,
				    int NumNodeTop,int NumNodeBottom);
static int Tre_GetPrevBrother (int NumNode);
static int Tre_GetNextBrother (int NumNode);

static void Tre_MoveLeftRightNode (Tre_MoveLeftRight_t LeftRight);

static void Tre_ExpandContractNode (Tre_ExpandContract_t ExpandContract);

static void Tre_SetNodeRangeWithAllChildren (unsigned NumNode,struct Tre_NodeRange *NodeRange);
static unsigned Tre_GetLastChild (int NumNode);

static void Tre_ShowFormToCreateNode (Tre_TreeType_t TreeType,long ParentNodCod);
static void Tre_ShowFormToChangeNode (Tre_TreeType_t TreeType,long NodCod);
static void Tre_ParsFormNode (void *NodCod);
static void Tre_ShowFormNode (const struct Tre_Node *Node,
			      const Dat_SetHMS SetHMS[Dat_NUM_START_END_TIME],
			      const char *Txt);

static void Tre_InsertNode (Tre_TreeType_t TreeType,
			    const struct Tre_Node *ParentNode,
		            struct Tre_Node *Node,const char *Txt);

/*****************************************************************************/
/**************************** List all tree nodes ****************************/
/*****************************************************************************/

void Tre_ShowTree (void)
  {
   Tre_TreeType_t TreeType;

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Show course program without highlighting any node *****/
   Tre_ShowAllNodes (TreeType,Tre_VIEW,-1L,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

void Tre_EditTree (void)
  {
   Tre_TreeType_t TreeType;

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Show course program without highlighting any node *****/
   Tre_ShowAllNodes (TreeType,Tre_EDIT_NODES,-1L,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/*************************** Show all tree nodes *****************************/
/*****************************************************************************/

void Tre_ShowAllNodes (Tre_TreeType_t TreeType,
		       Tre_ListingType_t ListingType,
                       long SelectedNodCod,long SelectedRscCod)
  {
   extern const char *Hlp_COURSE_Program;
   extern const char *Txt_COURSE_program;
   long ParentNodCod = -1L;	// Initialized to avoid warning
   unsigned NumNode;
   unsigned FormLevel = 0;	// Initialized to avoid warning
   struct Tre_Node Node;
   ConExp_ContractedOrExpanded_t ContractedOrExpanded;
   char *Title;
   static const char **Titles[Tre_NUM_TYPES] =
     {
      [Tre_PROGRAM] = &Txt_COURSE_program,
     };
   static void (*FunctionToDrawContextualIcons[Tre_NUM_LISTING_TYPES]) (void *Args) =
     {
      [Tre_PRINT			] = NULL,
      [Tre_VIEW				] = Tre_PutIconsListNodes,
      [Tre_EDIT_NODES			] = Tre_PutIconsEditNodes,
      [Tre_FORM_NEW_END_NODE		] = Tre_PutIconsEditNodes,
      [Tre_FORM_NEW_CHILD_NODE		] = Tre_PutIconsEditNodes,
      [Tre_FORM_EDIT_NODE		] = Tre_PutIconsEditNodes,
      [Tre_END_EDIT_NODE		] = Tre_PutIconsEditNodes,
      [Tre_RECEIVE_NODE			] = Tre_PutIconsEditNodes,
      [Tre_EDIT_PRG_RESOURCES		] = Tre_PutIconsEditNodes,
      [Tre_EDIT_PRG_RESOURCE_LINK	] = Tre_PutIconsEditNodes,
      [Tre_CHG_PRG_RESOURCE_LINK	] = Tre_PutIconsEditNodes,
      [Tre_END_EDIT_PRG_RESOURCES	] = Tre_PutIconsEditNodes,
     };

   /***** Create numbers and hidden levels *****/
   Tre_SetMaxNodeLevel (Tre_CalculateMaxNodeLevel ());
   Tre_CreateLevels ();

   /***** Compute form level *****/
   if (ListingType == Tre_FORM_NEW_CHILD_NODE)
     {
      ParentNodCod = SelectedNodCod;	// Node code here is parent of the node to create
      NumNode = Tre_GetNumNodeFromNodCod (SelectedNodCod);
      SelectedNodCod = Tre_GetNodCodFromNumNode (Tre_GetLastChild (NumNode));
      FormLevel = Tre_GetLevelFromNumNode (NumNode) + 1;
     }

   /***** Begin box *****/
   if (asprintf (&Title,*Titles[TreeType],
		 Gbl.Hierarchy.Node[Hie_CRS].ShrtName) < 0)
      Err_NotEnoughMemoryExit ();
   Box_BoxBegin (Title,FunctionToDrawContextualIcons[ListingType],NULL,
                 Hlp_COURSE_Program,Box_NOT_CLOSABLE);
   free (Title);

      /***** Table *****/
      HTM_TABLE_Begin ("TBL_SCROLL");

	 /***** Write all tree nodes *****/
	 for (NumNode = 0, The_ResetRowColor ();
	      NumNode < Tre_Gbl.List.NumNodes;
	      NumNode++)
	   {
	    /* Get data of this tree node */
	    Node.TreeType = TreeType;
	    Node.Hierarchy.NodCod = Tre_GetNodCodFromNumNode (NumNode);
	    Tre_GetNodeDataByCod (&Node);

	    /* Set if this level is expanded */
	    ContractedOrExpanded = Tre_DB_GetIfContractedOrExpandedNode (Node.Hierarchy.NodCod);
	    Tre_SetExpandedLevel (Node.Hierarchy.Level,ContractedOrExpanded);

	    /* Show this row only if all higher levels are expanded */
	    if (Tre_CheckIfAllHigherLevelsAreExpanded (Node.Hierarchy.Level))
	      {
	       /* Write row with this node */
	       Tre_WriteRowNode (TreeType,ListingType,NumNode,&Node,ContractedOrExpanded,
				 SelectedNodCod,SelectedRscCod);
               The_ChangeRowColor ();

	       /* Show form to create child node? */
	       if (ListingType == Tre_FORM_NEW_CHILD_NODE &&
		   Node.Hierarchy.NodCod == SelectedNodCod)
		 {
		  Tre_WriteRowToCreateNode (TreeType,ParentNodCod,FormLevel);
		  The_ChangeRowColor ();
		 }
	      }
	   }

	 /***** Create node at the end? *****/
	 if (ListingType == Tre_FORM_NEW_END_NODE)
	    Tre_WriteRowToCreateNode (TreeType,-1L,1);

	 /***** End table *****/
      HTM_TABLE_End ();

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free hidden levels and numbers *****/
   Tre_FreeLevels ();
  }

/*****************************************************************************/
/******************* Check if I can create tree nodes *********************/
/*****************************************************************************/

Usr_Can_t Tre_CheckIfICanEditTree (void)
  {
   return (Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
           Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM) ? Usr_CAN :
        					     Usr_CAN_NOT;
  }

/*****************************************************************************/
/**************** Put contextual icons in list of tree nodes *****************/
/*****************************************************************************/

static void Tre_PutIconsListNodes (__attribute__((unused)) void *Args)
  {
   static Fig_FigureType_t FigureType[Tre_NUM_TYPES] =
     {
      [Tre_PROGRAM] = Fig_COURSE_PROGRAMS,
     };

   /***** Put icon to edit tree *****/
   if (Tre_CheckIfICanEditTree () == Usr_CAN)
      Tre_PutIconToEditTree ();

   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (FigureType[Tre_PROGRAM]);
  }

/*****************************************************************************/
/*************** Put contextual icons in list of tree nodes ******************/
/*****************************************************************************/

static void Tre_PutIconsEditNodes (__attribute__((unused)) void *Args)
  {
   static Fig_FigureType_t FigureType[Tre_NUM_TYPES] =
     {
      [Tre_PROGRAM] = Fig_COURSE_PROGRAMS,
     };

   /***** Put icon to view program *****/
   Tre_PutIconToViewTree ();

   if (Tre_CheckIfICanEditTree () == Usr_CAN)
     {
      /***** Put icon to create a new tree node *****/
      Tre_PutIconToCreateNewNode ();

      /***** Put icon to view resource clipboard *****/
      Rsc_PutIconToViewClipboard ();
     }

   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (FigureType[Tre_PROGRAM]);
  }

/*****************************************************************************/
/************************** Put icon to edit tree ****************************/
/*****************************************************************************/

static void Tre_PutIconToEditTree (void)
  {
   static Act_Action_t NextAction[Tre_NUM_TYPES] =
     {
      [Tre_PROGRAM] = ActEdiPrg,
     };

   Ico_PutContextualIconToEdit (NextAction[Tre_PROGRAM],NULL,NULL,NULL);
  }

/*****************************************************************************/
/************************** Put icon to view tree ****************************/
/*****************************************************************************/

static void Tre_PutIconToViewTree (void)
  {
   static Act_Action_t NextAction[Tre_NUM_TYPES] =
     {
      [Tre_PROGRAM] = ActSeePrg,
     };

   Ico_PutContextualIconToView (NextAction[Tre_PROGRAM],NULL,NULL,NULL);
  }

/*****************************************************************************/
/******************** Put icon to create a new tree node *********************/
/*****************************************************************************/

static void Tre_PutIconToCreateNewNode (void)
  {
   long NodCod = -1L;
   static Act_Action_t NextAction[Tre_NUM_TYPES] =
     {
      [Tre_PROGRAM] = ActFrmNewPrgItm,
     };

   Ico_PutContextualIconToAdd (NextAction[Tre_PROGRAM],Tre_NODE_SECTION_ID,
                               Tre_PutParNodCod,&NodCod);
  }

/*****************************************************************************/
/************************** Show one tree node ****************************/
/*****************************************************************************/

static void Tre_WriteRowNode (Tre_TreeType_t TreeType,Tre_ListingType_t ListingType,
                              unsigned NumNode,struct Tre_Node *Node,
                              ConExp_ContractedOrExpanded_t ContractedOrExpanded,
                              long SelectedNodCod,
                              long SelectedRscCod)
  {
   extern const char *CloOpe_Class[CloOpe_NUM_CLOSED_OPEN][HidVis_NUM_HIDDEN_VISIBLE];
   extern const char *HidVis_PrgClass[HidVis_NUM_HIDDEN_VISIBLE];
   static unsigned UniqueId = 0;
   static Vie_ViewType_t ViewingOrEditingProgram[Tre_NUM_LISTING_TYPES] =
     {
      [Tre_PRINT               ] = Vie_VIEW,
      [Tre_VIEW                ] = Vie_VIEW,
      [Tre_EDIT_NODES          ] = Vie_EDIT,
      [Tre_FORM_NEW_END_NODE   ] = Vie_EDIT,
      [Tre_FORM_NEW_CHILD_NODE ] = Vie_EDIT,
      [Tre_FORM_EDIT_NODE      ] = Vie_EDIT,
      [Tre_END_EDIT_NODE       ] = Vie_EDIT,
      [Tre_RECEIVE_NODE        ] = Vie_EDIT,
      [Tre_EDIT_PRG_RESOURCES      ] = Vie_EDIT,
      [Tre_EDIT_PRG_RESOURCE_LINK  ] = Vie_EDIT,
      [Tre_CHG_PRG_RESOURCE_LINK] = Vie_EDIT,
      [Tre_END_EDIT_PRG_RESOURCES        ] = Vie_EDIT,
     };
   static const char *RowSpan[ConExp_NUM_CONTRACTED_EXPANDED] =
     {
      [ConExp_CONTRACTED] = "",			// Not expanded
      [ConExp_EXPANDED  ] = " rowspan=\"2\"",	// Expanded
     };
   HidVis_HiddenOrVisible_t HiddenOrVisible;
   char *Id;
   unsigned ColSpan;
   unsigned NumCol;
   char *TitleClass;
   Dat_StartEndTime_t StartEndTime;
   bool HighlightNode;

   /***** Check if this node should be shown as hidden *****/
   Tre_SetHiddenLevel (Node->Hierarchy.Level,Node->Hierarchy.HiddenOrVisible);
   switch (Node->Hierarchy.HiddenOrVisible)
     {
      case HidVis_VISIBLE:	// this node is not marked as hidden
         HiddenOrVisible = Tre_CheckIfAnyHigherLevelIsHidden (Node->Hierarchy.Level);
	 break;
      case HidVis_HIDDEN:	// this node is marked as hidden
      default:
         HiddenOrVisible = HidVis_HIDDEN;
	 break;
     }

   if (ViewingOrEditingProgram[ListingType] == Vie_EDIT ||
       HiddenOrVisible == HidVis_VISIBLE)
     {
      /***** Increase number in level *****/
      Tre_IncreaseNumberInLevel (Node->Hierarchy.Level);

      /***** Is this the node selected? *****/
      /*
      HighlightNode = Node->Hierarchy.NodCod == SelectedNodCod &&
		      (ListingType == Prg_FORM_EDIT_ITEM ||
		       ListingType == Prg_END_EDIT_ITEM);
      */
      HighlightNode = (Node->Hierarchy.NodCod == SelectedNodCod);

      /***** First row (title and dates) *****/
      HTM_TR_Begin (NULL);

	 /* Title CSS class */
	 Tre_SetTitleClass (&TitleClass,Node->Hierarchy.Level);

	 /* Indent depending on the level */
	 for (NumCol = 1;
	      NumCol < Node->Hierarchy.Level;
	      NumCol++)
	   {
	    HTM_TD_Begin ("class=\"%s\"%s",
	                  The_GetColorRows (),RowSpan[ContractedOrExpanded]);
	    HTM_TD_End ();
	   }

	 /* Expand/contract this tree node */
	 HTM_TD_Begin ("class=\"LT %s\"%s",
	               The_GetColorRows (),RowSpan[ContractedOrExpanded]);
	    Tre_PutIconToContractOrExpandNode (Node,ContractedOrExpanded,
					       ViewingOrEditingProgram[ListingType]);
	 HTM_TD_End ();

	 /* Forms to remove/edit this tree node */
	 if (ViewingOrEditingProgram[ListingType] == Vie_EDIT)
	   {
	    HTM_TD_Begin ("class=\"PRG_COL1 LT %s\"%s",
			  The_GetColorRows (),RowSpan[ContractedOrExpanded]);
	       Tre_PutFormsToRemEditOneNode (ListingType,NumNode,Node,HighlightNode);
	    HTM_TD_End ();
	   }

	 /* Node number */
	 HTM_TD_Begin ("class=\"PRG_NUM %s\"%s",
	               The_GetColorRows (),RowSpan[ContractedOrExpanded]);
	    HTM_DIV_Begin ("class=\"RT %s%s\"",
			   TitleClass,
			   HidVis_PrgClass[HiddenOrVisible]);
	       Tre_WriteNumNode (Node->Hierarchy.Level);
	    HTM_DIV_End ();
	 HTM_TD_End ();

	 /* Title */
	 ColSpan = (Tre_GetMaxNodeLevel () + 2) - Node->Hierarchy.Level;
	 switch (ListingType)
	   {
	    case Tre_PRINT:
	       HTM_TD_Begin ("colspan=\"%u\" class=\"PRG_MAIN\"",ColSpan);
	       break;
	    default:
	       HTM_TD_Begin ("colspan=\"%u\" class=\"PRG_MAIN %s\"",ColSpan,
			     The_GetColorRows ());

	       break;
	   }

	    if (HighlightNode)
	       HTM_ARTICLE_Begin (Tre_NODE_SECTION_ID);
	    HTM_DIV_Begin ("class=\"LT %s%s\"",
			   TitleClass,
			   HidVis_PrgClass[HiddenOrVisible]);
	       HTM_Txt (Node->Title);
	    HTM_DIV_End ();
	    if (HighlightNode)
	       HTM_ARTICLE_End ();
	 HTM_TD_End ();

	 /* Start/end date/time */
	 switch (ListingType)
	   {
	    case Tre_PRINT:
	       HTM_TD_Begin ("class=\"PRG_DATE RT\"");
	       break;
	    default:
	       HTM_TD_Begin ("class=\"PRG_DATE RT %s\"",The_GetColorRows ());
	       break;
	   }
	 UniqueId++;
	 for (StartEndTime  = (Dat_StartEndTime_t) 0;
	      StartEndTime <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	      StartEndTime++)
	   {
	    if (asprintf (&Id,"prg_date_%u_%u",(unsigned) StartEndTime,UniqueId) < 0)
	       Err_NotEnoughMemoryExit ();
	    HTM_DIV_Begin ("id=\"%s\" class=\"%s_%s%s\"",
			   Id,
			   CloOpe_Class[Node->ClosedOrOpen][HidVis_VISIBLE],The_GetSuffix (),
			   HidVis_PrgClass[HiddenOrVisible]);
	       Dat_WriteLocalDateHMSFromUTC (Id,Node->TimeUTC[StartEndTime],
					     Gbl.Prefs.DateFormat,Dat_SEPARATOR_COMMA,
					     Dat_WRITE_TODAY |
					     Dat_WRITE_DATE_ON_SAME_DAY |
					     Dat_WRITE_HOUR |
					     Dat_WRITE_MINUTE);
	    HTM_DIV_End ();
	    free (Id);
	   }
	 HTM_TD_End ();

	 /* Free title CSS class */
	 Tre_FreeTitleClass (TitleClass);

      HTM_TR_End ();

      /***** Second row (text and resources) *****/
      if (ContractedOrExpanded == ConExp_EXPANDED)
	{
	 HTM_TR_Begin (NULL);

	    /* Begin text and resources */
	    ColSpan++;
	    switch (ListingType)
	      {
	       case Tre_PRINT:
		  HTM_TD_Begin ("colspan=\"%u\" class=\"PRG_MAIN\"",ColSpan);
		  break;
	       default:
		  HTM_TD_Begin ("colspan=\"%u\" class=\"PRG_MAIN %s\"",
				ColSpan,The_GetColorRows ());
		  break;
	      }

	    /* Item text / form */
	    if (ListingType == Tre_FORM_EDIT_NODE && HighlightNode)
	       /* Form to change node title, dates and text */
	       Tre_ShowFormToChangeNode (TreeType,Node->Hierarchy.NodCod);
	    else
	       /* Text */
	       Tre_WriteNodeText (TreeType,Node->Hierarchy.NodCod,HiddenOrVisible);

	    /* List of resources */
	    PrgRsc_ListNodeResources (ListingType,Node,SelectedNodCod,SelectedRscCod);

	    /* End text and resources */
	    HTM_TD_End ();

	 HTM_TR_End ();
	}
     }
  }

/*****************************************************************************/
/************************ Put icon to expand a node **************************/
/*****************************************************************************/

static void Tre_PutIconToContractOrExpandNode (struct Tre_Node *Node,
                                               ConExp_ContractedOrExpanded_t ContractedOrExpanded,
                                               Vie_ViewType_t ViewType)
  {
   static Act_Action_t NextAction[ConExp_NUM_CONTRACTED_EXPANDED][Vie_NUM_VIEW_TYPES] =
     {
      [ConExp_CONTRACTED][Vie_VIEW] = ActExpSeePrgItm,	// Contracted, Not editing ==> action to expand
      [ConExp_CONTRACTED][Vie_EDIT] = ActExpEdiPrgItm,	// Contracted,     Editing ==> action to expand
      [ConExp_EXPANDED  ][Vie_VIEW] = ActConSeePrgItm,	// Expanded  , Not editing ==> action to contract
      [ConExp_EXPANDED  ][Vie_EDIT] = ActConEdiPrgItm,	// Expanded  ,     Editing ==> action to contract
     };
   static void (*PutContextualIcon[ConExp_NUM_CONTRACTED_EXPANDED]) (Act_Action_t NextAction,const char *Anchor,
								     void (*FuncPars) (void *Args),void *Args) =
     {
      [ConExp_CONTRACTED] = Ico_PutContextualIconToExpand,	// Contracted ==> function to expand
      [ConExp_EXPANDED  ] = Ico_PutContextualIconToContract,	// Expanded   ==> function to contract
     };

   /***** Icon to hide/unhide tree node *****/
   PutContextualIcon[ContractedOrExpanded] (NextAction[ContractedOrExpanded][ViewType],
					    // Prg_HIGHLIGHTED_SECTION_ID,
					    Tre_NODE_SECTION_ID,
					    Tre_PutParNodCod,&Node->Hierarchy.NodCod);
  }

/*****************************************************************************/
/**************************** Show node text *********************************/
/*****************************************************************************/

static void Tre_WriteNodeText (Tre_TreeType_t TreeType,
			       long NodCod,HidVis_HiddenOrVisible_t HiddenOrVisible)
  {
   extern const char *HidVis_PrgClass[HidVis_NUM_HIDDEN_VISIBLE];
   char Txt[Cns_MAX_BYTES_TEXT + 1];

   /* Text */
   Tre_DB_GetNodeTxt (TreeType,NodCod,Txt);
   Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
		     Txt,Cns_MAX_BYTES_TEXT,Str_DONT_REMOVE_SPACES);
   ALn_InsertLinks (Txt,Cns_MAX_BYTES_TEXT,60);	// Insert links
   HTM_DIV_Begin ("class=\"PAR PRG_TXT_%s%s\"",
		  The_GetSuffix (),HidVis_PrgClass[HiddenOrVisible]);
      HTM_Txt (Txt);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/**************************** Show node form *********************************/
/*****************************************************************************/

static void Tre_WriteRowToCreateNode (Tre_TreeType_t TreeType,
				      long ParentNodCod,unsigned FormLevel)
  {
   char *TitleClass;
   unsigned ColSpan;
   unsigned NumCol;

   /***** Title CSS class *****/
   Tre_SetTitleClass (&TitleClass,FormLevel);

   /***** Begin row *****/
   HTM_TR_Begin (NULL);

      /***** Column under expand/contract icon *****/
      HTM_TD_Begin ("class=\"LT %s\"",The_GetColorRows ());
      HTM_TD_End ();

      /***** Column under icons *****/
      HTM_TD_Begin ("class=\"PRG_COL1 LT %s\"",The_GetColorRows ());
      HTM_TD_End ();

      /***** Indent depending on the level *****/
      for (NumCol = 1;
	   NumCol < FormLevel;
	   NumCol++)
	{
	 HTM_TD_Begin ("class=\"%s\"",The_GetColorRows ());
	 HTM_TD_End ();
	}

      /***** Item number *****/
      HTM_TD_Begin ("class=\"PRG_NUM %s RT %s\"",
                    TitleClass,The_GetColorRows ());
	 Tre_WriteNumNewNode (FormLevel);
      HTM_TD_End ();

      /***** Show form to create new node as child *****/
      ColSpan = (Tre_GetMaxNodeLevel () + 4) - FormLevel;
      HTM_TD_Begin ("colspan=\"%u\" class=\"PRG_MAIN %s\"",
		    ColSpan,The_GetColorRows ());
         /* Form for node data */
	 HTM_ARTICLE_Begin (Tre_NODE_SECTION_ID);
	    Tre_ShowFormToCreateNode (TreeType,ParentNodCod);
	 HTM_ARTICLE_End ();
      HTM_TD_End ();

   /***** End row *****/
   HTM_TR_End ();

   /***** Free title CSS class *****/
   Tre_FreeTitleClass (TitleClass);
  }

/*****************************************************************************/
/**************** Set / free title class depending on level ******************/
/*****************************************************************************/

static void Tre_SetTitleClass (char **TitleClass,unsigned Level)
  {
   if (asprintf (TitleClass,"PRG_TIT_%u_%s",
		 Level < 5 ? Level :
			     5,
		 The_GetSuffix ()) < 0)
      Err_NotEnoughMemoryExit ();
  }

static void Tre_FreeTitleClass (char *TitleClass)
  {
   free (TitleClass);
  }

/*****************************************************************************/
/*************** Set and get maximum level in a course tree ******************/
/*****************************************************************************/

static void Tre_SetMaxNodeLevel (unsigned Level)
  {
   Tre_Gbl.MaxLevel = Level;
  }

static unsigned Tre_GetMaxNodeLevel (void)
  {
   return Tre_Gbl.MaxLevel;
  }

/*****************************************************************************/
/********* Calculate maximum level of indentation in a course tree ***********/
/*****************************************************************************/

static unsigned Tre_CalculateMaxNodeLevel (void)
  {
   unsigned NumNode;
   unsigned MaxLevel = 0;	// Return 0 if no nodes

   /***** Compute maximum level of all tree nodes *****/
   for (NumNode = 0;
	NumNode < Tre_Gbl.List.NumNodes;
	NumNode++)
      if (Tre_GetLevelFromNumNode (NumNode) > MaxLevel)
	 MaxLevel = Tre_GetLevelFromNumNode (NumNode);

   return MaxLevel;
  }

/*****************************************************************************/
/********************* Allocate memory for node numbers **********************/
/*****************************************************************************/

static void Tre_CreateLevels (void)
  {
   unsigned MaxLevel = Tre_GetMaxNodeLevel ();

   if (MaxLevel)
     {
      /***** Allocate memory for node numbers and initialize to 0 *****/
      /*
      Example:  2.5.2.1
                MaxLevel = 4
      Level Number
      ----- ------
        0    N.A. <--- Root level
        1     2
        2     5
        3     2
        4     1
        5     0	  <--- Used to create a new node
      */
      if ((Tre_Gbl.Levels = calloc (1 + MaxLevel + 1,
                                    sizeof (*Tre_Gbl.Levels))) == NULL)
	 Err_NotEnoughMemoryExit ();
     }
   else
      Tre_Gbl.Levels = NULL;
  }

/*****************************************************************************/
/*********************** Free memory for node numbers ************************/
/*****************************************************************************/

static void Tre_FreeLevels (void)
  {
   if (Tre_GetMaxNodeLevel () && Tre_Gbl.Levels)
     {
      /***** Free allocated memory for node numbers *****/
      free (Tre_Gbl.Levels);
      Tre_Gbl.Levels = NULL;
     }
  }

/*****************************************************************************/
/**************************** Increase number of node ************************/
/*****************************************************************************/

static void Tre_IncreaseNumberInLevel (unsigned Level)
  {
   /***** Increase number for this level *****/
   Tre_Gbl.Levels[Level    ].Number++;

   /***** Reset number for next level (children) *****/
   Tre_Gbl.Levels[Level + 1].Number = 0;
  }

/*****************************************************************************/
/****************** Get current number of node in a level ********************/
/*****************************************************************************/

static unsigned Tre_GetCurrentNumberInLevel (unsigned Level)
  {
   if (Tre_Gbl.Levels)
      return Tre_Gbl.Levels[Level].Number;

   return 0;
  }

/*****************************************************************************/
/******************** Write number of node in legal style ********************/
/*****************************************************************************/

static void Tre_WriteNumNode (unsigned Level)
  {
   HTM_Unsigned (Tre_GetCurrentNumberInLevel (Level));
  }

static void Tre_WriteNumNewNode (unsigned Level)
  {
   HTM_Unsigned (Tre_GetCurrentNumberInLevel (Level) + 1);
  }

/*****************************************************************************/
/********************** Set / Get if a level is hidden ***********************/
/*****************************************************************************/

static void Tre_SetExpandedLevel (unsigned Level,ConExp_ContractedOrExpanded_t ContractedOrExpanded)
  {
   if (Tre_Gbl.Levels)
      Tre_Gbl.Levels[Level].ContractedOrExpanded = ContractedOrExpanded;
  }

static void Tre_SetHiddenLevel (unsigned Level,HidVis_HiddenOrVisible_t HiddenOrVisible)
  {
   if (Tre_Gbl.Levels)
      Tre_Gbl.Levels[Level].HiddenOrVisible = HiddenOrVisible;
  }

static ConExp_ContractedOrExpanded_t Tre_GetExpandedLevel (unsigned Level)
  {
   /* Level 0 (root) is always expanded */
   if (Level == 0)
      return ConExp_EXPANDED;

   if (Tre_Gbl.Levels)
      return Tre_Gbl.Levels[Level].ContractedOrExpanded;

   return ConExp_CONTRACTED;
  }

static HidVis_HiddenOrVisible_t Tre_GetHiddenLevel (unsigned Level)
  {
   /* Level 0 (root) is always visible */
   if (Level == 0)
      return HidVis_VISIBLE;

   if (Tre_Gbl.Levels)
      return Tre_Gbl.Levels[Level].HiddenOrVisible;

   return HidVis_VISIBLE;
  }

/*****************************************************************************/
/********* Check if any level higher than the current one is hidden **********/
/*****************************************************************************/

static bool Tre_CheckIfAllHigherLevelsAreExpanded (unsigned CurrentLevel)
  {
   unsigned Level;

   for (Level = 1;
	Level < CurrentLevel;
	Level++)
      if (Tre_GetExpandedLevel (Level) == ConExp_CONTRACTED)	// Contracted?
         return false;	// A level is contracted. Not all are expanded

   return true;	// None is contracted. All are expanded
  }

static HidVis_HiddenOrVisible_t Tre_CheckIfAnyHigherLevelIsHidden (unsigned CurrentLevel)
  {
   unsigned Level;

   for (Level = 1;
	Level < CurrentLevel;
	Level++)
      if (Tre_GetHiddenLevel (Level) == HidVis_HIDDEN)
         return HidVis_HIDDEN;

   return HidVis_VISIBLE;	// None is hidden. All are visible.
  }

/*****************************************************************************/
/**************** Put a link (form) to edit one tree node *****************/
/*****************************************************************************/

static void Tre_PutFormsToRemEditOneNode (Tre_ListingType_t ListingType,
                                          unsigned NumNode,
                                          struct Tre_Node *Node,
                                          bool HighlightNode)
  {
   extern const char *Txt_Movement_not_allowed;
   static Act_Action_t ActionHideUnhide[HidVis_NUM_HIDDEN_VISIBLE] =
     {
      [HidVis_HIDDEN ] = ActUnhPrgItm,	// Hidden ==> action to unhide
      [HidVis_VISIBLE] = ActHidPrgItm,	// Visible ==> action to hide
     };
   char StrItemIndex[Cns_MAX_DIGITS_UINT + 1];

   /***** Initialize node index string *****/
   snprintf (StrItemIndex,sizeof (StrItemIndex),"%u",Node->Hierarchy.NodInd);

   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_TCH:
      case Rol_SYS_ADM:
	 /***** Icon to remove tree node *****/
	 Ico_PutContextualIconToRemove (ActReqRemPrgItm,NULL,
	                                Tre_PutParNodCod,&Node->Hierarchy.NodCod);

	 /***** Icon to hide/unhide tree node *****/
	 Ico_PutContextualIconToHideUnhide (ActionHideUnhide,Tre_NODE_SECTION_ID /*Prg_HIGHLIGHTED_SECTION_ID */,
					    Tre_PutParNodCod,&Node->Hierarchy.NodCod,
					    Node->Hierarchy.HiddenOrVisible);

	 /***** Icon to edit tree node *****/
	 if (ListingType == Tre_FORM_EDIT_NODE && HighlightNode)
	    Ico_PutContextualIconToView (ActSeePrgItm,Tre_NODE_SECTION_ID,
					 Tre_PutParNodCod,&Node->Hierarchy.NodCod);
	 else
	    Ico_PutContextualIconToEdit (ActFrmChgPrgItm,Tre_NODE_SECTION_ID,
					 Tre_PutParNodCod,&Node->Hierarchy.NodCod);

	 /***** Icon to add a new child node inside this node *****/
	 Ico_PutContextualIconToAdd (ActFrmNewPrgItm,Tre_NODE_SECTION_ID,
	                             Tre_PutParNodCod,&Node->Hierarchy.NodCod);

	 HTM_BR ();

	 /***** Icon to move up the node *****/
	 if (Tre_CheckIfMoveUpIsAllowed (NumNode))
	    Lay_PutContextualLinkOnlyIcon (ActUp_PrgItm,Tre_NODE_SECTION_ID /* Prg_HIGHLIGHTED_SECTION_ID */,
	                                   Tre_PutParNodCod,&Node->Hierarchy.NodCod,
					   "arrow-up.svg",Ico_BLACK);
	 else
	    Ico_PutIconOff ("arrow-up.svg",Ico_BLACK,Txt_Movement_not_allowed);

	 /***** Icon to move down the node *****/
	 if (Tre_CheckIfMoveDownIsAllowed (NumNode))
	    Lay_PutContextualLinkOnlyIcon (ActDwnPrgItm,Tre_NODE_SECTION_ID /* Prg_HIGHLIGHTED_SECTION_ID */,
	                                   Tre_PutParNodCod,&Node->Hierarchy.NodCod,
					   "arrow-down.svg",Ico_BLACK);
	 else
	    Ico_PutIconOff ("arrow-down.svg",Ico_BLACK,Txt_Movement_not_allowed);

	 /***** Icon to move left node (increase level) *****/
	 if (Tre_CheckIfMoveLeftIsAllowed (NumNode))
	    Lay_PutContextualLinkOnlyIcon (ActLftPrgItm,Tre_NODE_SECTION_ID /* Prg_HIGHLIGHTED_SECTION_ID */,
	                                   Tre_PutParNodCod,&Node->Hierarchy.NodCod,
					   "arrow-left.svg",Ico_BLACK);
	 else
            Ico_PutIconOff ("arrow-left.svg",Ico_BLACK,Txt_Movement_not_allowed);

	 /***** Icon to move right node (indent, decrease level) *****/
	 if (Tre_CheckIfMoveRightIsAllowed (NumNode))
	    Lay_PutContextualLinkOnlyIcon (ActRgtPrgItm,Tre_NODE_SECTION_ID /* Prg_HIGHLIGHTED_SECTION_ID */,
	                                   Tre_PutParNodCod,&Node->Hierarchy.NodCod,
					   "arrow-right.svg",Ico_BLACK);
	 else
            Ico_PutIconOff ("arrow-right.svg",Ico_BLACK,Txt_Movement_not_allowed);
	 break;
      case Rol_STD:
      case Rol_NET:
	 break;
      default:
         break;
     }
  }

/*****************************************************************************/
/*********************** Check if node can be moved up ***********************/
/*****************************************************************************/

static bool Tre_CheckIfMoveUpIsAllowed (unsigned NumNode)
  {
   /***** Trivial check: if node is the first one, move up is not allowed *****/
   if (NumNode == 0)
      return false;

   /***** Move up is allowed if the node has brothers before it *****/
   // NumItem >= 1
   return Tre_GetLevelFromNumNode (NumNode - 1) >=
	  Tre_GetLevelFromNumNode (NumNode    );
  }

/*****************************************************************************/
/********************** Check if node can be moved down **********************/
/*****************************************************************************/

static bool Tre_CheckIfMoveDownIsAllowed (unsigned NumNode)
  {
   unsigned i;
   unsigned Level;

   /***** Trivial check: if node is the last one, move up is not allowed *****/
   if (NumNode >= Tre_Gbl.List.NumNodes - 1)
      return false;

   /***** Move down is allowed if the node has brothers after it *****/
   // NumItem + 1 < Prg_Gbl.List.NumItems
   Level = Tre_GetLevelFromNumNode (NumNode);
   for (i = NumNode + 1;
	i < Tre_Gbl.List.NumNodes;
	i++)
     {
      if (Tre_GetLevelFromNumNode (i) == Level)
	 return true;	// Next brother found
      if (Tre_GetLevelFromNumNode (i) < Level)
	 return false;	// Next lower level found ==> there are no more brothers
     }
   return false;	// End reached ==> there are no more brothers
  }

/*****************************************************************************/
/******************* Check if node can be moved to the left ******************/
/*****************************************************************************/

static bool Tre_CheckIfMoveLeftIsAllowed (unsigned NumNode)
  {
   /***** Move left is allowed if the node has parent *****/
   return Tre_GetLevelFromNumNode (NumNode) > 1;
  }

/*****************************************************************************/
/****************** Check if node can be moved to the right ******************/
/*****************************************************************************/

static bool Tre_CheckIfMoveRightIsAllowed (unsigned NumNode)
  {
   /***** If node is the first, move right is not allowed *****/
   if (NumNode == 0)
      return false;

   /***** Move right is allowed if the node has brothers before it *****/
   // NumItem >= 2
   return Tre_GetLevelFromNumNode (NumNode - 1) >=
	  Tre_GetLevelFromNumNode (NumNode    );
  }

/*****************************************************************************/
/********************** Params used to edit a tree node **********************/
/*****************************************************************************/

void Tre_PutParNodCod (void *NodCod)
  {
   if (NodCod)
      ParCod_PutPar (ParCod_Nod,*((long *) NodCod));
  }

// Node type must be set before calling this function
void Tre_GetPars (struct Tre_Node *Node)
  {
   /***** Clear all node data except type *****/
   Tre_ResetNode (Node);

   /****** Parameters specific for each type of tree *****/
   switch (Node->TreeType)
     {
      case Tre_PROGRAM:
	 /***** Try to get node resource *****/
	 Node->Resource.Hierarchy.RscCod = ParCod_GetPar (ParCod_Rsc);

	 /***** Get data of the tree node from database *****/
	 PrgRsc_GetResourceDataByCod (Node);
	 break;
      default:
	 break;
     }

   if (Node->Hierarchy.NodCod <= 0)	// No node specified
      /***** Try to get data of the tree node *****/
      Node->Hierarchy.NodCod = ParCod_GetPar (ParCod_Nod);

   /***** Get data of the tree node from database *****/
   Tre_GetNodeDataByCod (Node);
  }

/*****************************************************************************/
/*************************** List all tree nodes *****************************/
/*****************************************************************************/

void Tre_GetListNodes (Tre_TreeType_t TreeType)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumItem;

   if (Tre_Gbl.List.IsRead)
      Tre_FreeListNodes ();

   /***** Get list of tree nodes from database *****/
   if ((Tre_Gbl.List.NumNodes = Tre_DB_GetListNodes (TreeType,&mysql_res))) // Nodes found...
     {
      /***** Create list of tree nodes *****/
      if ((Tre_Gbl.List.Nodes = calloc (Tre_Gbl.List.NumNodes,
				        sizeof (*Tre_Gbl.List.Nodes))) == NULL)
         Err_NotEnoughMemoryExit ();

      /***** Get the tree nodes codes *****/
      for (NumItem = 0;
	   NumItem < Tre_Gbl.List.NumNodes;
	   NumItem++)
        {
         /* Get row */
         row = mysql_fetch_row (mysql_res);

         /* Get code of the tree node (row[0]) */
         if ((Tre_Gbl.List.Nodes[NumItem].NodCod = Str_ConvertStrCodToLongCod (row[0])) <= 0)
            Err_WrongItemExit ();

         /* Get index of the tree node (row[1])
            and level of the tree node (row[2]) */
         Tre_Gbl.List.Nodes[NumItem].NodInd = Str_ConvertStrToUnsigned (row[1]);
         Tre_Gbl.List.Nodes[NumItem].Level  = Str_ConvertStrToUnsigned (row[2]);

	 /* Get whether the tree node is hidden or not (row[3]) */
	 Tre_Gbl.List.Nodes[NumItem].HiddenOrVisible = HidVid_GetHiddenOrVisible (row[3][0]);
        }
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   Tre_Gbl.List.IsRead = true;
  }

/*****************************************************************************/
/****************** Get tree node data using its code *********************/
/*****************************************************************************/
// Node type must be set before calling this function

static void Tre_GetNodeDataByCod (struct Tre_Node *Node)
  {
   MYSQL_RES *mysql_res;
   unsigned NumRows;

   if (Node->Hierarchy.NodCod > 0)
     {
      /***** Build query *****/
      NumRows = Tre_DB_GetNodeDataByCod (Node,&mysql_res);

      /***** Get data of tree node *****/
      Tre_GetNodeDataFromRow (&mysql_res,Node,NumRows);
     }
   else
      /***** Clear all node data except type *****/
      Tre_ResetNode (Node);
  }

/*****************************************************************************/
/************************* Get tree node data *****************************/
/*****************************************************************************/
// Node type must be set before calling this function

static void Tre_GetNodeDataFromRow (MYSQL_RES **mysql_res,
                                    struct Tre_Node *Node,
                                    unsigned NumRows)
  {
   MYSQL_ROW row;

   /***** Get data of tree node from database *****/
   if (NumRows) // Item found...
     {
      /* Get row */
      row = mysql_fetch_row (*mysql_res);
      /*
      NodCod					row[0]
      NodInd					row[1]
      Level					row[2]
      Hidden					row[3]
      UsrCod					row[4]
      UNIX_TIMESTAMP(StartTime)			row[5]
      UNIX_TIMESTAMP(EndTime)			row[6]
      NOW() BETWEEN StartTime AND EndTime	row[7]
      Title					row[8]
      */

      /* Get code of the tree node (row[0]) */
      Node->Hierarchy.NodCod = Str_ConvertStrCodToLongCod (row[0]);

      /* Get index of the tree node (row[1])
         and level of the tree node (row[2]) */
      Node->Hierarchy.NodInd = Str_ConvertStrToUnsigned (row[1]);
      Node->Hierarchy.Level = Str_ConvertStrToUnsigned (row[2]);

      /* Get whether the tree node is hidden or not (row[3]) */
      Node->Hierarchy.HiddenOrVisible = HidVid_GetHiddenOrVisible (row[3][0]);

      /* Get author of the tree node (row[4]) */
      Node->UsrCod = Str_ConvertStrCodToLongCod (row[4]);

      /* Get start date (row[5] holds the start UTC time)
         and end date   (row[6] holds the end   UTC time) */
      Node->TimeUTC[Dat_STR_TIME] = Dat_GetUNIXTimeFromStr (row[5]);
      Node->TimeUTC[Dat_END_TIME] = Dat_GetUNIXTimeFromStr (row[6]);

      /* Get whether the tree node is open or closed (row(7)) */
      Node->ClosedOrOpen = CloOpe_GetClosedOrOpenFrom01 (row[7][0]);

      /* Get the title of the tree node (row[8]) */
      Str_Copy (Node->Title,row[8],sizeof (Node->Title) - 1);
     }
   else
      /***** Clear all node data except type *****/
      Tre_ResetNode (Node);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (mysql_res);
  }

/*****************************************************************************/
/************************* Free list of tree nodes ***************************/
/*****************************************************************************/

void Tre_FreeListNodes (void)
  {
   if (Tre_Gbl.List.IsRead && Tre_Gbl.List.Nodes)
     {
      /***** Free memory used by the list of tree nodes *****/
      free (Tre_Gbl.List.Nodes);
      Tre_Gbl.List.Nodes = NULL;
      Tre_Gbl.List.NumNodes = 0;
      Tre_Gbl.List.IsRead = false;
     }
  }

/*****************************************************************************/
/************************ Clear all tree node data ************************/
/*****************************************************************************/

void Tre_ResetNode (struct Tre_Node *Node)
  {
   // Node->TreeType is not reset
   Node->Hierarchy.NodCod = -1L;
   Node->Hierarchy.NodInd = 0;
   Node->Hierarchy.Level  = 0;
   Node->Hierarchy.HiddenOrVisible = HidVis_VISIBLE;
   Node->UsrCod = -1L;
   Node->TimeUTC[Dat_STR_TIME] =
   Node->TimeUTC[Dat_END_TIME] = (time_t) 0;
   Node->ClosedOrOpen = CloOpe_CLOSED;
   Node->Title[0] = '\0';
   if (Node->TreeType == Tre_PROGRAM)
      Rsc_ResetResource (&Node->Resource);
  }

/*****************************************************************************/
/************* Get number of tree node in list from node code ****************/
/*****************************************************************************/

unsigned Tre_GetNumNodeFromNodCod (long NodCod)
  {
   unsigned NumNode;

   /***** List of nodes must be filled *****/
   if (!Tre_Gbl.List.IsRead || Tre_Gbl.List.Nodes == NULL)
      Err_WrongItemsListExit ();

   /***** Find node code in list *****/
   for (NumNode = 0;
	NumNode < Tre_Gbl.List.NumNodes;
	NumNode++)
      if (Tre_GetNodCodFromNumNode (NumNode) == NodCod)	// Found!
	 return NumNode;

   /***** Not found *****/
   Err_WrongItemExit ();
   return 0;	// Not reached
  }

/*****************************************************************************/
/******************** Get node code from number of node **********************/
/*****************************************************************************/

inline long Tre_GetNodCodFromNumNode (unsigned NumNode)
  {
   return Tre_Gbl.List.Nodes[NumNode].NodCod;
  }

/*****************************************************************************/
/******************** Get node index from number of node *********************/
/*****************************************************************************/

inline unsigned Tre_GetNodIndFromNumNode (unsigned NumNode)
  {
   return Tre_Gbl.List.Nodes[NumNode].NodInd;
  }

/*****************************************************************************/
/****************** Get level of node from number of node ********************/
/*****************************************************************************/

inline unsigned Tre_GetLevelFromNumNode (unsigned NumNode)
  {
   return Tre_Gbl.List.Nodes[NumNode].Level;
  }

/*****************************************************************************/
/************* Ask for confirmation of removing a tree node ***************/
/*****************************************************************************/

void Tre_ReqRemNode (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_the_item_X;
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);
   if (Node.Hierarchy.NodCod <= 0)
      Err_WrongItemExit ();

   /***** Show question and button to remove the tree node *****/
   Ale_ShowAlertRemove (ActRemPrgItm,NULL,
                        Tre_PutParNodCod,&Node.Hierarchy.NodCod,
			Txt_Do_you_really_want_to_remove_the_item_X,
                        Node.Title);

   /***** Show tree node highlighting subtree *****/
   Tre_ShowAllNodes (TreeType,Tre_EDIT_NODES,Node.Hierarchy.NodCod,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/******************* Remove a tree node and its children ******************/
/*****************************************************************************/

void Tre_RemoveNode (void)
  {
   extern const char *Txt_Item_X_removed;
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;
   struct Tre_NodeRange ToRemove;

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);
   if (Node.Hierarchy.NodCod <= 0)
      Err_WrongItemExit ();

   /***** Indexes of items *****/
   Tre_SetNodeRangeWithAllChildren (Tre_GetNumNodeFromNodCod (Node.Hierarchy.NodCod),
				    &ToRemove);

   /***** Remove tree nodes *****/
   Tre_DB_RemoveNodeRange (TreeType,&ToRemove);

   /***** Write message to show the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Item_X_removed,Node.Title);

   /***** Update list of tree nodes *****/
   Tre_FreeListNodes ();
   Tre_GetListNodes (TreeType);

   /***** Show course program without highlighting any node *****/
   Tre_ShowAllNodes (TreeType,Tre_EDIT_NODES,-1L,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/***************************** Hide a tree node ***************************/
/*****************************************************************************/

void Tre_HideNode (void)
  {
   Tre_HideOrUnhideNode (HidVis_HIDDEN);
  }

void Tre_UnhideNode (void)
  {
   Tre_HideOrUnhideNode (HidVis_VISIBLE);
  }

static void Tre_HideOrUnhideNode (HidVis_HiddenOrVisible_t HiddenOrVisible)
  {
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);
   if (Node.Hierarchy.NodCod <= 0)
      Err_WrongItemExit ();

   /***** Hide/unhide tree node *****/
   Tre_DB_HideOrUnhideNode (&Node,HiddenOrVisible);

   /***** Show tree nodes highlighting subtree *****/
   Tre_ShowAllNodes (TreeType,Tre_EDIT_NODES,Node.Hierarchy.NodCod,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/*********** Move up/down position of a subtree in a course tree *************/
/*****************************************************************************/

void Tre_MoveUpNode (void)
  {
   Tre_MoveUpDownNode (Tre_MOVE_UP);
  }

void Tre_MoveDownNode (void)
  {
   Tre_MoveUpDownNode (Tre_MOVE_DOWN);
  }

static void Tre_MoveUpDownNode (Tre_MoveUpDown_t UpDown)
  {
   extern const char *Txt_Movement_not_allowed;
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;
   unsigned NumNode;
   bool Success = false;
   static bool (*CheckIfAllowed[Tre_NUM_MOVEMENTS_UP_DOWN])(unsigned NumNode) =
     {
      [Tre_MOVE_UP  ] = Tre_CheckIfMoveUpIsAllowed,
      [Tre_MOVE_DOWN] = Tre_CheckIfMoveDownIsAllowed,
     };

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);
   if (Node.Hierarchy.NodCod <= 0)
      Err_WrongItemExit ();

   /***** Move up/down node *****/
   NumNode = Tre_GetNumNodeFromNodCod (Node.Hierarchy.NodCod);
   if (CheckIfAllowed[UpDown] (NumNode))
     {
      /* Exchange subtrees */
      switch (UpDown)
        {
	 case Tre_MOVE_UP:
            Success = Tre_ExchangeNodeRanges (TreeType,Tre_GetPrevBrother (NumNode),NumNode);
            break;
	 case Tre_MOVE_DOWN:
            Success = Tre_ExchangeNodeRanges (TreeType,NumNode,Tre_GetNextBrother (NumNode));
            break;
        }
     }
   if (Success)
     {
      /* Update list of tree nodes */
      Tre_FreeListNodes ();
      Tre_GetListNodes (TreeType);

      /* Show tree nodes highlighting subtree */
      Tre_ShowAllNodes (TreeType,Tre_EDIT_NODES,Node.Hierarchy.NodCod,-1L);
     }
   else
     {
      /* Show course program without highlighting any node */
      Ale_ShowAlert (Ale_WARNING,Txt_Movement_not_allowed);
      Tre_ShowAllNodes (TreeType,Tre_EDIT_NODES,-1L,-1L);
     }

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/**** Exchange the order of two consecutive subtrees in a course program *****/
/*****************************************************************************/
// Return true if success

static bool Tre_ExchangeNodeRanges (Tre_TreeType_t TreeType,
				    int NumNodeTop,int NumNodeBottom)
  {
   struct Tre_NodeRange Top;
   struct Tre_NodeRange Bottom;
   unsigned DiffBegin;
   unsigned DiffEnd;

   if (NumNodeTop    >= 0 &&
       NumNodeBottom >= 0)
     {
      Tre_SetNodeRangeWithAllChildren (NumNodeTop   ,&Top   );
      Tre_SetNodeRangeWithAllChildren (NumNodeBottom,&Bottom);
      DiffBegin = Bottom.Begin - Top.Begin;
      DiffEnd   = Bottom.End   - Top.End;

      /***** Lock table to make the move atomic *****/
      Tre_DB_LockTableNodes ();

      /***** Exchange indexes of items *****/
      // This implementation works with non continuous indexes
      /*
      Example:
      Top.Begin    =  5
		   = 10
      Top.End      = 17
      Bottom.Begin = 28
      Bottom.End   = 49

      DiffBegin = 28 -  5 = 23;
      DiffEnd   = 49 - 17 = 32;

                                Step 1            Step 2            Step 3          (Equivalent to)
              +------+------+   +------+------+   +------+------+   +------+------+ +------+------+
              |NodInd|NodCod|   |NodInd|NodCod|   |NodInd|NodCod|   |NodInd|NodCod| |NodInd|NodCod|
              +------+------+   +------+------+   +------+------+   +------+------+ +------+------+
Top.Begin:    |     5|   218|-->|--> -5|   218|-->|--> 37|   218|   |    37|   218| |     5|   221|
              |    10|   219|-->|-->-10|   219|-->|--> 42|   219|   |    42|   219| |    26|   222|
Top.End:      |    17|   220|-->|-->-17|   220|-->|--> 49|   220|   |    49|   220| |    37|   218|
Bottom.Begin: |    28|   221|-->|-->-28|   221|   |   -28|   221|-->|-->  5|   221| |    42|   219|
Bottom.End:   |    49|   222|-->|-->-49|   222|   |   -49|   222|-->|--> 26|   222| |    49|   220|
              +------+------+   +------+------+   +------+------+   +------+------+ +------+------+
      */
      /* Step 1: Change all indexes involved to negative,
		 necessary to preserve unique index (CrsCod,NodInd) */
      Tre_DB_UpdateIndexRange (TreeType,
				 (long) 0            ,	// NodInd=-NodInd
                                 (long) Top.Begin    ,
                                 (long) Bottom.End   );	// All indexes in both parts

      /* Step 2: Increase top indexes */
      Tre_DB_UpdateIndexRange (TreeType,
				 (long) DiffEnd      ,	// NodInd=-NodInd+DiffEnd
                               -((long) Top.End     ),
                               -((long) Top.Begin   ));	// All indexes in top part

      /* Step 3: Decrease bottom indexes */
      Tre_DB_UpdateIndexRange (TreeType,
			       -((long) DiffBegin   ),	// NodInd=-NodInd-DiffBegin
                               -((long) Bottom.End  ),
                               -((long) Bottom.Begin));	// All indexes in bottom part

      /***** Unlock table *****/
      DB_UnlockTables ();

      return true;	// Success
     }

   return false;	// No success
  }

/*****************************************************************************/
/******** Get previous brother node to a given node in current course ********/
/*****************************************************************************/
// Return -1 if no previous brother

static int Tre_GetPrevBrother (int NumNode)
  {
   unsigned Level;
   int i;

   /***** Trivial check: if node is the first one, there is no previous brother *****/
   if (NumNode <= 0 ||
       NumNode >= (int) Tre_Gbl.List.NumNodes)
      return -1;

   /***** Get previous brother before node *****/
   // 1 <= NumItem < Prg_Gbl.List.NumItems
   Level = Tre_GetLevelFromNumNode (NumNode);
   for (i  = NumNode - 1;
	i >= 0;
	i--)
     {
      if (Tre_GetLevelFromNumNode (i) == Level)
	 return i;	// Previous brother before node found
      if (Tre_GetLevelFromNumNode (i) < Level)
	 return -1;	// Previous lower level found ==> there are no brothers before node
     }
   return -1;	// Start reached ==> there are no brothers before node
  }

/*****************************************************************************/
/********** Get next brother node to a given node in current course **********/
/*****************************************************************************/
// Return -1 if no next brother

static int Tre_GetNextBrother (int NumNode)
  {
   unsigned Level;
   int i;

   /***** Trivial check: if node is the last one, there is no next brother *****/
   if (NumNode < 0 ||
       NumNode >= (int) Tre_Gbl.List.NumNodes - 1)
      return -1;

   /***** Get next brother after node *****/
   // 0 <= NumItem < Prg_Gbl.List.NumItems - 1
   Level = Tre_GetLevelFromNumNode (NumNode);
   for (i = NumNode + 1;
	i < (int) Tre_Gbl.List.NumNodes;
	i++)
     {
      if (Tre_GetLevelFromNumNode (i) == Level)
	 return i;	// Next brother found
      if (Tre_GetLevelFromNumNode (i) < Level)
	 return -1;	// Next lower level found ==> there are no brothers after node
     }
   return -1;	// End reached ==> there are no brothers after node
  }

/*****************************************************************************/
/************** Move a subtree to left/right in a course program *************/
/*****************************************************************************/

void Tre_MoveLeftNode (void)
  {
   Tre_MoveLeftRightNode (Tre_MOVE_LEFT);
  }

void Tre_MoveRightNode (void)
  {
   Tre_MoveLeftRightNode (Tre_MOVE_RIGHT);
  }

static void Tre_MoveLeftRightNode (Tre_MoveLeftRight_t LeftRight)
  {
   extern const char *Txt_Movement_not_allowed;
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;
   unsigned NumNode;
   struct Tre_NodeRange ToMove;
   static bool (*CheckIfAllowed[Tre_NUM_MOVEMENTS_LEFT_RIGHT])(unsigned NumNode) =
     {
      [Tre_MOVE_LEFT ] = Tre_CheckIfMoveLeftIsAllowed,
      [Tre_MOVE_RIGHT] = Tre_CheckIfMoveRightIsAllowed,
     };

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);
   if (Node.Hierarchy.NodCod <= 0)
      Err_WrongItemExit ();

   /***** Move up/down node *****/
   NumNode = Tre_GetNumNodeFromNodCod (Node.Hierarchy.NodCod);
   if (CheckIfAllowed[LeftRight](NumNode))
     {
      /* Indexes of items */
      Tre_SetNodeRangeWithAllChildren (NumNode,&ToMove);

      /* Move node and its children to left or right */
      Tre_DB_MoveLeftRightNodeRange (TreeType,&ToMove,LeftRight);

      /* Update list of tree nodes */
      Tre_FreeListNodes ();
      Tre_GetListNodes (TreeType);

      /* Show tree nodes highlighting subtree */
      Tre_ShowAllNodes (TreeType,Tre_EDIT_NODES,Node.Hierarchy.NodCod,-1L);
     }
   else
     {
      /* Show course program without highlighting any node */
      Ale_ShowAlert (Ale_WARNING,Txt_Movement_not_allowed);
      Tre_ShowAllNodes (TreeType,Tre_EDIT_NODES,-1L,-1L);
     }

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/************** Move a subtree to left/right in a course program *************/
/*****************************************************************************/

void Tre_ExpandNode (void)
  {
   Tre_ExpandContractNode (Tre_EXPAND);
  }

void Tre_ContractNode (void)
  {
   Tre_ExpandContractNode (Tre_CONTRACT);
  }

static void Tre_ExpandContractNode (Tre_ExpandContract_t ExpandContract)
  {
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;
   Tre_ListingType_t ListingType;

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);
   if (Node.Hierarchy.NodCod <= 0)
      Err_WrongItemExit ();

   /***** Add/remove node to/from table of expanded nodes *****/
   switch (ExpandContract)
     {
      case Tre_EXPAND:
	 Tre_DB_InsertNodeInExpandedNodes (Node.Hierarchy.NodCod);
	 break;
      case Tre_CONTRACT:
	 Tre_DB_RemoveNodeFromExpandedNodes (Node.Hierarchy.NodCod);
	 break;
     }

   /***** Show tree nodes highlighting subtree *****/
   switch (Gbl.Action.Act)
     {
      case ActExpEdiPrgItm:
      case ActConEdiPrgItm:
	 ListingType = Tre_EDIT_NODES;
	 break;
      case ActExpSeePrgItm:
      case ActConSeePrgItm:
      default:
	 ListingType = Tre_VIEW;
	 break;
     }
   Tre_ShowAllNodes (TreeType,ListingType,Node.Hierarchy.NodCod,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

static void Tre_SetNodeRangeWithAllChildren (unsigned NumNode,struct Tre_NodeRange *NodeRange)
  {
   /***** List of nodes must be filled *****/
   if (!Tre_Gbl.List.IsRead)
      Err_WrongItemsListExit ();

   /***** Number of node must be in the correct range *****/
   if (NumNode >= Tre_Gbl.List.NumNodes)
      Err_WrongItemExit ();

   /***** Range includes this node and all its children *****/
   NodeRange->Begin = Tre_GetNodIndFromNumNode (NumNode);
   NodeRange->End   = Tre_GetNodIndFromNumNode (Tre_GetLastChild (NumNode));
  }

/*****************************************************************************/
/********************** Get last child in current course *********************/
/*****************************************************************************/

static unsigned Tre_GetLastChild (int NumNode)
  {
   unsigned Level;
   int i;

   /***** Trivial check: if node is wrong, there are no children *****/
   if (NumNode < 0 ||
       NumNode >= (int) Tre_Gbl.List.NumNodes)
      Err_WrongItemExit ();

   /***** Get next brother after node *****/
   // 0 <= NumItem < Prg_Gbl.List.NumItems
   Level = Tre_GetLevelFromNumNode (NumNode);
   for (i = NumNode + 1;
	i < (int) Tre_Gbl.List.NumNodes;
	i++)
     {
      if (Tre_GetLevelFromNumNode (i) <= Level)
	 return i - 1;	// Last child found
     }
   return Tre_Gbl.List.NumNodes - 1;	// End reached ==> all items after the given node are its children
  }

/*****************************************************************************/
/********* List tree nodes when click on view an node after edition **********/
/*****************************************************************************/

void Tre_ViewNodeAfterEdit (void)
  {
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);

   /***** Show current tree nodes, if any *****/
   Tre_ShowAllNodes (TreeType,Tre_END_EDIT_NODE,Node.Hierarchy.NodCod,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/*********** List tree nodes with a form to change a given item ***********/
/*****************************************************************************/

void Tre_ReqChangeNode (void)
  {
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);

   /***** If node is contracted ==> expand it *****/
   if (Tre_DB_GetIfContractedOrExpandedNode (Node.Hierarchy.NodCod) == ConExp_CONTRACTED)	// If contracted...
      Tre_DB_InsertNodeInExpandedNodes (Node.Hierarchy.NodCod);					// ...expand it

   /***** Show current tree nodes, if any *****/
   Tre_ShowAllNodes (TreeType,Tre_FORM_EDIT_NODE,Node.Hierarchy.NodCod,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/************* List tree nodes with a form to create a new node **************/
/*****************************************************************************/

void Tre_ReqCreateNode (void)
  {
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);

   /***** Add node to table of expanded nodes
          to ensure that child items are displayed *****/
   Tre_DB_InsertNodeInExpandedNodes (Node.Hierarchy.NodCod);

   /***** Show current tree nodes, if any *****/
   Tre_ShowAllNodes (TreeType,
		     Node.Hierarchy.NodCod > 0 ? Tre_FORM_NEW_CHILD_NODE :
	                                         Tre_FORM_NEW_END_NODE,
	             Node.Hierarchy.NodCod,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/***************** Put a form to create a new tree node *******************/
/*****************************************************************************/

static void Tre_ShowFormToCreateNode (Tre_TreeType_t TreeType,long ParentNodCod)
  {
   struct Tre_Node ParentNode;	// Parent node
   struct Tre_Node Node;
   static const Dat_SetHMS SetHMS[Dat_NUM_START_END_TIME] =
     {
      [Dat_STR_TIME] = Dat_HMS_TO_000000,
      [Dat_END_TIME] = Dat_HMS_TO_235959
     };

   /***** Get data of the parent tree node from database *****/
   ParentNode.TreeType = TreeType;
   ParentNode.Hierarchy.NodCod = ParentNodCod;
   Tre_GetNodeDataByCod (&ParentNode);

   /***** Initialize to empty node *****/
   Node.TreeType = ParentNode.TreeType;
   Tre_ResetNode (&Node);	// Clear all node data except type
   Node.TimeUTC[Dat_STR_TIME] = Dat_GetStartExecutionTimeUTC ();
   Node.TimeUTC[Dat_END_TIME] = Node.TimeUTC[Dat_STR_TIME] + (2 * 60 * 60);	// +2 hours
   Node.ClosedOrOpen = CloOpe_OPEN;

   /***** Show pending alerts */
   Ale_ShowAlerts (NULL);

   /***** Begin form to create *****/
   Frm_BeginFormTable (ActNewPrgItm,NULL,
                       Tre_ParsFormNode,&ParentNode.Hierarchy.NodCod,
                       "TBL_WIDE");

      /***** Show form *****/
      Tre_ShowFormNode (&Node,SetHMS,NULL);

   /***** End form to create *****/
   Frm_EndFormTable (Btn_CREATE_BUTTON);
  }

static void Tre_ShowFormToChangeNode (Tre_TreeType_t TreeType,long NodCod)
  {
   struct Tre_Node Node;
   char Txt[Cns_MAX_BYTES_TEXT + 1];
   static const Dat_SetHMS SetHMS[Dat_NUM_START_END_TIME] =
     {
      [Dat_STR_TIME] = Dat_HMS_DO_NOT_SET,
      [Dat_END_TIME] = Dat_HMS_DO_NOT_SET
     };

   /***** Get data of the tree node from database *****/
   Node.TreeType = TreeType;
   Node.Hierarchy.NodCod = NodCod;
   Tre_GetNodeDataByCod (&Node);
   Tre_DB_GetNodeTxt (TreeType,Node.Hierarchy.NodCod,Txt);

   /***** Begin form to change *****/
   Frm_BeginFormTable (ActChgPrgItm,Tre_NODE_SECTION_ID /* Prg_HIGHLIGHTED_SECTION_ID */,
                       Tre_ParsFormNode,&Node.Hierarchy.NodCod,"TBL_WIDE");

      /***** Show form *****/
      Tre_ShowFormNode (&Node,SetHMS,Txt);

   /***** End form to change *****/
   Frm_EndFormTable (Btn_CONFIRM_BUTTON);
  }

static void Tre_ParsFormNode (void *NodCod)
  {
   ParCod_PutPar (ParCod_Nod,*(long *) NodCod);
  }

/*****************************************************************************/
/***************** Put a form to create a new tree node *******************/
/*****************************************************************************/

static void Tre_ShowFormNode (const struct Tre_Node *Node,
			      const Dat_SetHMS SetHMS[Dat_NUM_START_END_TIME],
		              const char *Txt)
  {
   extern const char *Txt_Title;
   extern const char *Txt_Description;

   /***** Node title *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RM","Title",Txt_Title);

      /* Data */
      HTM_TD_Begin ("class=\"LM\"");
	 HTM_INPUT_TEXT ("Title",Tre_MAX_CHARS_NODE_TITLE,Node->Title,
			 HTM_REQUIRED,
			 "id=\"Title\""
			 " class=\"PRG_TITLE_DESCRIPTION_WIDTH INPUT_%s\"",
			 The_GetSuffix ());
      HTM_TD_End ();

   HTM_TR_End ();

   /***** Node start and end dates *****/
   Dat_PutFormStartEndClientLocalDateTimes (Node->TimeUTC,
					    Dat_FORM_SECONDS_ON,
					    SetHMS);

   /***** Node text *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT","Txt",Txt_Description);

      /* Data */
      HTM_TD_Begin ("class=\"LT\"");
	 HTM_TEXTAREA_Begin (HTM_NO_ATTR,
			     "id=\"Txt\" name=\"Txt\" rows=\"10\""
			     " class=\"PRG_TITLE_DESCRIPTION_WIDTH INPUT_%s\"",
			     The_GetSuffix ());
	    if (Txt)
	       if (Txt[0])
		  HTM_Txt (Txt);
	 HTM_TEXTAREA_End ();
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/*************** Receive form to change an existing tree node ****************/
/*****************************************************************************/

void Tre_ReceiveChgNode (void)
  {
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;
   char Description[Cns_MAX_BYTES_TEXT + 1];

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);
   if (Node.Hierarchy.NodCod <= 0)
      Err_WrongItemExit ();

   /***** Get start/end date-times *****/
   Node.TimeUTC[Dat_STR_TIME] = Dat_GetTimeUTCFromForm (Dat_STR_TIME);
   Node.TimeUTC[Dat_END_TIME] = Dat_GetTimeUTCFromForm (Dat_END_TIME);

   /***** Get node title *****/
   Par_GetParText ("Title",Node.Title,Tre_MAX_BYTES_NODE_TITLE);

   /***** Get node text *****/
   Par_GetParHTML ("Txt",Description,Cns_MAX_BYTES_TEXT);	// Store in HTML format (not rigorous)

   /***** Adjust dates *****/
   if (Node.TimeUTC[Dat_STR_TIME] == 0)
      Node.TimeUTC[Dat_STR_TIME] = Dat_GetStartExecutionTimeUTC ();
   if (Node.TimeUTC[Dat_END_TIME] == 0)
      Node.TimeUTC[Dat_END_TIME] = Node.TimeUTC[Dat_STR_TIME] + 2 * 60 * 60;	// +2 hours

   /***** Update existing node *****/
   Tre_DB_UpdateNode (&Node,Description);

   /***** Show tree nodes highlighting the node just changed *****/
   Tre_ShowAllNodes (TreeType,Tre_RECEIVE_NODE,Node.Hierarchy.NodCod,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/******************* Receive form to create a new tree node ******************/
/*****************************************************************************/

void Tre_ReceiveNewNode (void)
  {
   Tre_TreeType_t TreeType;
   struct Tre_Node Node;		// Parent node
   struct Tre_Node NewNode;		// Node data received from form
   char Description[Cns_MAX_BYTES_TEXT + 1];

   /***** Set tree type *****/
   switch (Act_GetSuperAction (Gbl.Action.Act))
     {
      case ActSeePrg:
         TreeType = Tre_PROGRAM;
         break;
      default:
	 Err_WrongActionExit ();
	 break;
     }

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);

   /***** Get tree node *****/
   Node.TreeType = TreeType;
   Tre_GetPars (&Node);
   // If node code <= 0 ==> this is the first node in the program

   /***** Set new node code *****/
   NewNode.Hierarchy.NodCod = -1L;
   NewNode.Hierarchy.Level = Node.Hierarchy.Level + 1;	// Create as child

   /***** Get start/end date-times *****/
   NewNode.TimeUTC[Dat_STR_TIME] = Dat_GetTimeUTCFromForm (Dat_STR_TIME);
   NewNode.TimeUTC[Dat_END_TIME] = Dat_GetTimeUTCFromForm (Dat_END_TIME);

   /***** Get node title *****/
   Par_GetParText ("Title",NewNode.Title,Tre_MAX_BYTES_NODE_TITLE);

   /***** Get node text *****/
   Par_GetParHTML ("Txt",Description,Cns_MAX_BYTES_TEXT);	// Store in HTML format (not rigorous)

   /***** Adjust dates *****/
   if (NewNode.TimeUTC[Dat_STR_TIME] == 0)
      NewNode.TimeUTC[Dat_STR_TIME] = Dat_GetStartExecutionTimeUTC ();
   if (NewNode.TimeUTC[Dat_END_TIME] == 0)
      NewNode.TimeUTC[Dat_END_TIME] = NewNode.TimeUTC[Dat_STR_TIME] + 2 * 60 * 60;	// +2 hours

   /***** Create a new tree node *****/
   Tre_InsertNode (TreeType,&Node,&NewNode,Description);

   /* Update list of tree nodes */
   Tre_FreeListNodes ();
   Tre_GetListNodes (TreeType);

   /***** Show tree nodes highlighting the node just created *****/
   Tre_ShowAllNodes (TreeType,Tre_EDIT_NODES,NewNode.Hierarchy.NodCod,-1L);

   /***** Free list of tree nodes *****/
   Tre_FreeListNodes ();
  }

/*****************************************************************************/
/*************** Insert a new node as a child of a parent node ***************/
/*****************************************************************************/

static void Tre_InsertNode (Tre_TreeType_t TreeType,
			    const struct Tre_Node *ParentNode,
		            struct Tre_Node *Node,const char *Txt)
  {
   unsigned NumNodeLastChild;

   /***** Lock table to create tree node *****/
   Tre_DB_LockTableNodes ();

   /***** Get list of tree nodes *****/
   Tre_GetListNodes (TreeType);
   if (Tre_Gbl.List.NumNodes)	// There are nodes
     {
      if (ParentNode->Hierarchy.NodCod > 0)	// Parent specified
	{
	 /***** Calculate where to insert *****/
	 NumNodeLastChild = Tre_GetLastChild (Tre_GetNumNodeFromNodCod (ParentNode->Hierarchy.NodCod));
	 if (NumNodeLastChild < Tre_Gbl.List.NumNodes - 1)
	   {
	    /***** New node will be inserted after last child of parent *****/
	    Node->Hierarchy.NodInd = Tre_GetNodIndFromNumNode (NumNodeLastChild + 1);

	    /***** Move down all indexes of after last child of parent *****/
	    Tre_DB_MoveDownNodes (TreeType,Node->Hierarchy.NodInd);
	   }
	 else
	    /***** New node will be inserted at the end *****/
	    Node->Hierarchy.NodInd = Tre_GetNodIndFromNumNode (Tre_Gbl.List.NumNodes - 1) + 1;

	 /***** Child ==> parent level + 1 *****/
         Node->Hierarchy.Level = ParentNode->Hierarchy.Level + 1;
	}
      else	// No parent specified
	{
	 /***** New tree node will be inserted at the end *****/
	 Node->Hierarchy.NodInd = Tre_GetNodIndFromNumNode (Tre_Gbl.List.NumNodes - 1) + 1;

	 /***** First level *****/
         Node->Hierarchy.Level = 1;
	}
     }
   else		// There are no nodes
     {
      /***** New tree node will be inserted as the first one *****/
      Node->Hierarchy.NodInd = 1;

      /***** First level *****/
      Node->Hierarchy.Level = 1;
     }

   /***** Insert new tree node *****/
   Node->TreeType = TreeType;
   Node->Hierarchy.NodCod = Tre_DB_InsertNode (Node,Txt);

   /***** Unlock table *****/
   DB_UnlockTables ();

   /***** Free list items *****/
   Tre_FreeListNodes ();
  }
