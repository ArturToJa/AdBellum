// Editor-only tools for ALS_AnimBP.
//
// adb.Anim.RewireTransitionRules [apply]
//   Rewires ALS_AnimBP transition rules so they read the C++ variables in
//   UALSCharacterAnimInstance::TransitionRules (fast path, no Blueprint VM), or turns on the
//   automatic "animation finished" rule. Without "apply" it only lists what it would change.
//   The anim BP is compiled afterwards but not saved.

#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectHash.h"
#include "Animation/AnimBlueprint.h"
#include "AnimationTransitionGraph.h"
#include "AnimGraphNode_TransitionResult.h"
#include "AnimStateTransitionNode.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ScopedTransaction.h"
#include "Character/Animation/ALSCharacterAnimInstance.h"
#include "Library/ALSAnimationStructLibrary.h"

#define LOCTEXT_NAMESPACE "ALSV4_CPPEditor"

DEFINE_LOG_CATEGORY_STATIC(LogALSRewire, Log, All);

namespace ALSRewire
{
	static const TCHAR* AnimBPPath = TEXT("/ALSV4_CPP/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/ALS_AnimBP.ALS_AnimBP");

	enum class EAction : uint8
	{
		Rule,		// read TransitionRules.<Member>
		Member,		// read <Variable>.<Member> (existing anim instance struct)
		AutoRule	// automatic rule based on the from-state's asset player (time remaining <= 0)
	};

	struct FEntry
	{
		const TCHAR* Graph;		// graph path inside the anim BP (after "ALS_AnimBP:")
		EAction Action;
		const TCHAR* Variable;	// anim instance variable for EAction::Member
		const TCHAR* Member;	// bool member to read
		bool bNegate;
	};

	// State machine graph paths
#define LTC TEXT("AimOffsetBehaviors.AnimGraphNode_StateMachine_0.Aim Offset Behavior States.AnimStateNode_1.Look Towards Camera.AnimGraphNode_StateMachine_0.Look Towards Camera States.")
#define MM TEXT("BaseLayer.AnimGraphNode_StateMachine_9.Main Movement States.")
#define JUMP TEXT("BaseLayer.AnimGraphNode_StateMachine_9.Main Movement States.AnimStateNode_2.Jump.AnimGraphNode_StateMachine_0.Jump States.")
#define MG TEXT("BaseLayer.AnimGraphNode_StateMachine_11.Main Grounded States.")
#define LD TEXT("BaseLayer.AnimGraphNode_StateMachine_0.(N) Locomotion Detail.")
#define CLF TEXT("BaseLayer.AnimGraphNode_StateMachine_2.(CLF) Locomotion States.")
#define NLOCO TEXT("BaseLayer.AnimGraphNode_StateMachine_6.(N) Locomotion States.")
#define STOP TEXT("BaseLayer.AnimGraphNode_StateMachine_6.(N) Locomotion States.AnimStateNode_4.(N) Stop.AnimGraphNode_StateMachine_0.(N) Stop States.")
#define CRAWL TEXT("BaseLayer.AnimGraphNode_StateMachine_3.(Crawl CLF) States.")
#define OVL TEXT("OverlayLayer.AnimGraphNode_StateMachine_5.Overlay States.")
#define RIFLE TEXT("OverlayLayer.AnimGraphNode_StateMachine_5.Overlay States.AnimStateNode_15.Rifle.AnimGraphNode_StateMachine_1.Rifle States.")
#define RIFLE1 TEXT("OverlayLayer.AnimGraphNode_StateMachine_5.Overlay States.AnimStateNode_15.Rifle.AnimGraphNode_StateMachine_0.Rifle States_1.")
#define P1H TEXT("OverlayLayer.AnimGraphNode_StateMachine_5.Overlay States.AnimStateNode_16.Pistol 1H.AnimGraphNode_StateMachine_0.Pistol 1H States.")
#define P2H TEXT("OverlayLayer.AnimGraphNode_StateMachine_5.Overlay States.AnimStateNode_17.Pistol 2H.AnimGraphNode_StateMachine_1.Pistol 2H States.")
#define P2H1 TEXT("OverlayLayer.AnimGraphNode_StateMachine_5.Overlay States.AnimStateNode_17.Pistol 2H.AnimGraphNode_StateMachine_0.Pistol 2H States_1.")
#define BOW TEXT("OverlayLayer.AnimGraphNode_StateMachine_5.Overlay States.AnimStateNode_18.Bow.AnimGraphNode_StateMachine_0.Bow States.")
#define RPG TEXT("OverlayLayer.AnimGraphNode_StateMachine_5.Overlay States.AnimStateNode_7.RPG.AnimGraphNode_StateMachine_1.RPG States.")
#define RPG1 TEXT("OverlayLayer.AnimGraphNode_StateMachine_5.Overlay States.AnimStateNode_7.RPG.AnimGraphNode_StateMachine_0.RPG States_1.")
#define NDIR TEXT("(N) CycleBlending.AnimGraphNode_StateMachine_1.(N) Directional States.")
#define CLFDIR TEXT("(CLF) CycleBlending.AnimGraphNode_StateMachine_1.CLF_Directional States.")

#define RULE(InPath, InBool) {InPath, EAction::Rule, nullptr, TEXT(InBool), false}
#define NOT_RULE(InPath, InBool) {InPath, EAction::Rule, nullptr, TEXT(InBool), true}
#define MEMBER(InPath, InVariable, InBool) {InPath, EAction::Member, TEXT(InVariable), TEXT(InBool), false}
#define AUTO_RULE(Path) {Path, EAction::AutoRule, nullptr, nullptr, false}

	// Every ALS_AnimBP transition rule that compiled with "uses Blueprint to update its values" (85)
	static const FEntry Entries[] = {
		// Look Towards Camera States
		RULE(LTC TEXT("AnimStateTransitionNode_67.Transition"), "bAimLeftBack"),
		RULE(LTC TEXT("AnimStateTransitionNode_68.Transition"), "bAimRightBack"),
		RULE(LTC TEXT("AnimStateTransitionNode_73.Transition"), "bAimForward"),
		RULE(LTC TEXT("AnimStateTransitionNode_75.Transition"), "bNoOffsetDelayDone"),
		RULE(LTC TEXT("AnimStateTransitionNode_77.AnimationTransitionGraph_0"), "bAimForwardRightBackNotFull"),
		RULE(LTC TEXT("AnimStateTransitionNode_78.AnimationTransitionGraph_0"), "bAimRightBackForwardsNotFull"),
		RULE(LTC TEXT("AnimStateTransitionNode_79.AnimationTransitionGraph_0"), "bAimForwardLeftBackNotFull"),
		RULE(LTC TEXT("AnimStateTransitionNode_80.AnimationTransitionGraph_0"), "bAimLeftBackForwardsNotFull"),

		// Jump States
		RULE(JUMP TEXT("AnimStateTransitionNode_22.Transition"), "bFeetPositionNegative"),
		NOT_RULE(JUMP TEXT("AnimStateTransitionNode_23.Transition"), "bFeetPositionNegative"),
		AUTO_RULE(JUMP TEXT("AnimStateTransitionNode_24.Transition")),
		AUTO_RULE(JUMP TEXT("AnimStateTransitionNode_28.Transition")),

		// Main Movement States
		AUTO_RULE(MM TEXT("AnimStateTransitionNode_3.AnimationTransitionGraph_0")),
		RULE(MM TEXT("AnimStateTransitionNode_24.AnimationTransitionGraph_0"), "bNotGroundedOrNotStanding"),
		RULE(MM TEXT("AnimStateTransitionNode_10.AnimationTransitionGraph_0"), "bNotGroundedOrNotStanding"),
		RULE(MM TEXT("AnimStateTransitionNode_1.AnimationTransitionGraph_0"), "bHasInputOrRotatingOrFast"),
		RULE(MM TEXT("AnimStateTransitionNode_26.AnimationTransitionGraph_0"), "bHasInputOrRotating"),

		// Main Grounded States
		RULE(MG TEXT("AnimStateTransitionNode_10.AnimationTransitionGraph_0"), "bBasePoseCLFLow"),
		NOT_RULE(MG TEXT("AnimStateTransitionNode_0.AnimationTransitionGraph_0"), "bBasePoseCLFLow"),
		RULE(MG TEXT("AnimStateTransitionNode_13.Transition"), "bShouldMoveOrRotating"),
		RULE(MG TEXT("AnimStateTransitionNode_25.AnimationTransitionGraph_1"), "bShouldMoveOrRotating"),
		MEMBER(MG TEXT("AnimStateTransitionNode_21.AnimationTransitionGraph_1"), "Stance", "Crouching_"),
		MEMBER(MG TEXT("AnimStateTransitionNode_16.Transition"), "Stance", "Standing_"),
		MEMBER(MG TEXT("AnimStateConduitNode_5.(P) -> (C)"), "MovementAction", "None_"),
		MEMBER(MG TEXT("AnimStateConduitNode_6.(C) -> (P)"), "MovementAction", "None_"),
		RULE(MG TEXT("AnimStateTransitionNode_15.Transition"), "bIsCrawling"),
		RULE(MG TEXT("AnimStateTransitionNode_17.Transition"), "bIsCrawling"),
		RULE(MG TEXT("AnimStateTransitionNode_26.AnimationTransitionGraph_1"), "bIsCrawling"),
		NOT_RULE(MG TEXT("AnimStateTransitionNode_30.AnimationTransitionGraph_1"), "bIsCrawling"),
		NOT_RULE(MG TEXT("AnimStateTransitionNode_32.AnimationTransitionGraph_1"), "bIsCrawling"),

		// (N) Locomotion Detail
		RULE(LD TEXT("AnimStateTransitionNode_15.AnimationTransitionGraph_0"), "bWalkingWithLowGaitCurve"),
		AUTO_RULE(LD TEXT("AnimStateTransitionNode_16.AnimationTransitionGraph_0")),
		AUTO_RULE(LD TEXT("AnimStateTransitionNode_33.Transition")),
		AUTO_RULE(LD TEXT("AnimStateTransitionNode_3.Transition")),
		AUTO_RULE(LD TEXT("AnimStateTransitionNode_7.Transition")),
		RULE(LD TEXT("AnimStateTransitionNode_1.Transition"), "bPivotAfterMinTime"),
		RULE(LD TEXT("AnimStateTransitionNode_2.Transition"), "bPivotAfterMinTime"),
		RULE(LD TEXT("AnimStateTransitionNode_4.Transition"), "bNotWalkingGroundedFull"),
		RULE(LD TEXT("AnimStateTransitionNode_0.Transition"), "bNotWalkingGroundedPartial"),
		RULE(LD TEXT("AnimStateTransitionNode_5.Transition"), "bNLocomotionFull"),
		NOT_RULE(LD TEXT("AnimStateTransitionNode_6.Transition"), "bNLocomotionFull"),

		// Stops
		RULE(CLF TEXT("AnimStateTransitionNode_0.Transition"), "bCLFStopFromMoving"),
		RULE(CLF TEXT("AnimStateTransitionNode_1.Transition"), "bCLFStopDone"),
		RULE(NLOCO TEXT("AnimStateTransitionNode_8.Transition"), "bNStopFromMoving"),
		RULE(NLOCO TEXT("AnimStateTransitionNode_10.Transition"), "bNStopDone"),
		RULE(CRAWL TEXT("AnimStateTransitionNode_10.Transition"), "bCrawlStopFromMoving"),
		RULE(CRAWL TEXT("AnimStateTransitionNode_11.Transition"), "bCrawlStopDone"),
		RULE(STOP TEXT("AnimStateTransitionNode_12.Transition"), "bFeetPositionFar"),
		NOT_RULE(STOP TEXT("AnimStateTransitionNode_13.Transition"), "bFeetPositionFar"),
		RULE(STOP TEXT("AnimStateTransitionNode_14.Transition"), "bFeetPositionNegative"),
		RULE(STOP TEXT("AnimStateTransitionNode_15.Transition"), "bFeetPositionPositive"),

		// Overlay States
		RULE(OVL TEXT("AnimStateTransitionNode_11.Transition"), "bIsOverlayRPG"),
		NOT_RULE(OVL TEXT("AnimStateTransitionNode_14.Transition"), "bIsOverlayRPG"),

		// Weapon overlays (Ready <-> Relaxed)
		RULE(RIFLE TEXT("AnimStateTransitionNode_29.AnimationTransitionGraph_0"), "bRifleReadyMinTimeOrAiming"),
		RULE(RIFLE TEXT("AnimStateTransitionNode_28.AnimationTransitionGraph_0"), "bRifleRelaxByCurve"),
		RULE(RIFLE TEXT("AnimStateTransitionNode_30.AnimationTransitionGraph_0"), "bRifleRelaxByMoving"),
		RULE(RIFLE1 TEXT("AnimStateTransitionNode_28.AnimationTransitionGraph_0"), "bRifle1RelaxByCurve"),
		RULE(RIFLE1 TEXT("AnimStateTransitionNode_30.AnimationTransitionGraph_0"), "bRifle1RelaxByMoving"),
		RULE(RIFLE1 TEXT("AnimStateTransitionNode_31.AnimationTransitionGraph_0"), "bInAirOrSprinting"),
		RULE(P1H TEXT("AnimStateTransitionNode_42.AnimationTransitionGraph_1"), "bPistol1HRelaxByCurve"),
		RULE(P1H TEXT("AnimStateTransitionNode_44.AnimationTransitionGraph_1"), "bPistol1HRelaxByMoving"),
		RULE(P1H TEXT("AnimStateTransitionNode_45.AnimationTransitionGraph_1"), "bInAirOrSprinting"),
		RULE(P2H TEXT("AnimStateTransitionNode_54.AnimationTransitionGraph_0"), "bPistol2HRelaxByCurve"),
		RULE(P2H TEXT("AnimStateTransitionNode_56.AnimationTransitionGraph_0"), "bPistol2HRelaxByMoving"),
		RULE(P2H TEXT("AnimStateTransitionNode_57.AnimationTransitionGraph_0"), "bInAirOrSprinting"),
		RULE(P2H1 TEXT("AnimStateTransitionNode_54.AnimationTransitionGraph_0"), "bPistol2H1RelaxByCurve"),
		RULE(P2H1 TEXT("AnimStateTransitionNode_56.AnimationTransitionGraph_0"), "bPistol2H1RelaxByMoving"),
		RULE(P2H1 TEXT("AnimStateTransitionNode_57.AnimationTransitionGraph_0"), "bInAirOrSprinting"),
		RULE(BOW TEXT("AnimStateTransitionNode_16.AnimationTransitionGraph_1"), "bBowRelaxByRotation"),
		RULE(BOW TEXT("AnimStateTransitionNode_0.AnimationTransitionGraph_1"), "bBowRelaxByMoving"),
		RULE(BOW TEXT("AnimStateTransitionNode_1.AnimationTransitionGraph_1"), "bInAirOrSprinting"),
		RULE(RPG TEXT("AnimStateTransitionNode_8.Transition"), "bInAirOrSprinting"),
		RULE(RPG TEXT("AnimStateTransitionNode_9.Transition"), "bRPGRelaxByMoving"),
		RULE(RPG TEXT("AnimStateTransitionNode_10.Transition"), "bRPGRelaxByCurve"),
		RULE(RPG1 TEXT("AnimStateTransitionNode_8.Transition"), "bInAirOrSprinting"),
		RULE(RPG1 TEXT("AnimStateTransitionNode_9.Transition"), "bRPG1RelaxByMoving"),
		RULE(RPG1 TEXT("AnimStateTransitionNode_10.Transition"), "bRPG1RelaxByCurve"),

		// Directional cycle blending
		RULE(NDIR TEXT("AnimStateTransitionNode_16.Transition"), "bHipBiasLeftNoCrossing"),
		RULE(NDIR TEXT("AnimStateTransitionNode_28.Transition"), "bHipBiasRightNoCrossing"),
		RULE(NDIR TEXT("AnimStateTransitionNode_14.Transition"), "bNDirCenteredFromLB"),
		RULE(NDIR TEXT("AnimStateTransitionNode_15.Transition"), "bNDirCenteredFromRB"),
		RULE(CLFDIR TEXT("AnimStateTransitionNode_17.Transition"), "bHipBiasLeftNoCrossing"),
		RULE(CLFDIR TEXT("AnimStateTransitionNode_23.Transition"), "bHipBiasRightNoCrossing"),
		RULE(CLFDIR TEXT("AnimStateTransitionNode_15.Transition"), "bCLFDirCenteredFromRB"),
		RULE(CLFDIR TEXT("AnimStateTransitionNode_22.Transition"), "bCLFDirCenteredFromLF"),
	};

#undef RULE
#undef NOT_RULE
#undef MEMBER
#undef AUTO_RULE
#undef LTC
#undef MM
#undef JUMP
#undef MG
#undef LD
#undef CLF
#undef NLOCO
#undef STOP
#undef CRAWL
#undef OVL
#undef RIFLE
#undef RIFLE1
#undef P1H
#undef P2H
#undef P2H1
#undef BOW
#undef RPG
#undef RPG1
#undef NDIR
#undef CLFDIR

	/** Graph path relative to the anim BP, e.g. "BaseLayer.AnimGraphNode_StateMachine_9.Main Movement States...." */
	static FString RelativeGraphPath(const UEdGraph* Graph)
	{
		const FString Path = Graph->GetPathName();
		int32 Colon = INDEX_NONE;
		return Path.FindChar(TEXT(':'), Colon) ? Path.Mid(Colon + 1) : Path;
	}

	/**
	 * State machine anim graph node names (AnimGraphNode_StateMachine_N) can change when the asset is
	 * reloaded, while graph names (e.g. "Rifle States") and transition node names stay the same.
	 * Replaces the number after every "AnimGraphNode_StateMachine_" with '*'.
	 */
	static FString NormalizeMachineNodeNames(const FString& Path)
	{
		static const FString Token = TEXT("AnimGraphNode_StateMachine_");
		FString Result;
		int32 Index = 0;
		while (Index < Path.Len())
		{
			const int32 Found = Path.Find(Token, ESearchCase::CaseSensitive, ESearchDir::FromStart, Index);
			if (Found == INDEX_NONE)
			{
				Result += Path.Mid(Index);
				break;
			}
			Result += Path.Mid(Index, Found - Index) + Token + TEXT("*");
			Index = Found + Token.Len();
			while (Index < Path.Len() && FChar::IsDigit(Path[Index]))
			{
				++Index;
			}
		}
		return Result;
	}

	/** Resolves the struct and bool member an entry reads; returns false and fills OutError when missing */
	static bool ResolveSource(const FEntry& Entry, FName& OutVariable, UScriptStruct*& OutStruct, FString& OutError)
	{
		OutVariable = Entry.Action == EAction::Rule ? FName(TEXT("TransitionRules")) : FName(Entry.Variable);
		const FStructProperty* StructProperty = FindFProperty<FStructProperty>(UALSCharacterAnimInstance::StaticClass(), OutVariable);
		if (!StructProperty)
		{
			OutError = FString::Printf(TEXT("variable '%s' not found on UALSCharacterAnimInstance"), *OutVariable.ToString());
			return false;
		}
		OutStruct = StructProperty->Struct;
		if (!FindFProperty<FBoolProperty>(OutStruct, FName(Entry.Member)))
		{
			OutError = FString::Printf(TEXT("bool '%s' not found in %s"), Entry.Member, *OutStruct->GetName());
			return false;
		}
		return true;
	}

	/** Removes every node except the result node and disconnects the result */
	static void ClearGraph(UAnimBlueprint* AnimBP, UAnimationTransitionGraph* Graph, UAnimGraphNode_TransitionResult* Result)
	{
		TArray<UEdGraphNode*> Nodes = Graph->Nodes;
		for (UEdGraphNode* Node : Nodes)
		{
			if (Node && Node != Result)
			{
				FBlueprintEditorUtils::RemoveNode(AnimBP, Node, true);
			}
		}
		if (UEdGraphPin* ResultPin = Result->FindPin(TEXT("bCanEnterTransition")))
		{
			ResultPin->BreakAllPinLinks();
			ResultPin->DefaultValue = TEXT("false");
		}
	}

	static UEdGraphPin* FindFirstPin(UEdGraphNode* Node, EEdGraphPinDirection Direction, FName Category)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == Direction && Pin->PinType.PinCategory == Category)
			{
				return Pin;
			}
		}
		return nullptr;
	}

	/** Wires <Variable> -> Break <Struct> -> [NOT] -> Result. Returns false and fills OutError on failure */
	static bool WireRule(UAnimationTransitionGraph* Graph, UAnimGraphNode_TransitionResult* Result, FName Variable,
	                     UScriptStruct* Struct, FName Member, bool bNegate, FString& OutError)
	{
		const UEdGraphSchema* Schema = Graph->GetSchema();
		UEdGraphPin* ResultPin = Result->FindPin(TEXT("bCanEnterTransition"));
		if (!ResultPin)
		{
			OutError = TEXT("result node has no bCanEnterTransition pin");
			return false;
		}

		const int32 X = Result->NodePosX;
		const int32 Y = Result->NodePosY;

		FGraphNodeCreator<UK2Node_VariableGet> GetCreator(*Graph);
		UK2Node_VariableGet* GetNode = GetCreator.CreateNode(false);
		GetNode->VariableReference.SetSelfMember(Variable);
		GetNode->NodePosX = X - (bNegate ? 700 : 520);
		GetNode->NodePosY = Y;
		GetCreator.Finalize();

		FGraphNodeCreator<UK2Node_BreakStruct> BreakCreator(*Graph);
		UK2Node_BreakStruct* BreakNode = BreakCreator.CreateNode(false);
		BreakNode->StructType = Struct;
		BreakNode->bMadeAfterOverridePinRemoval = true;
		BreakNode->NodePosX = X - (bNegate ? 450 : 270);
		BreakNode->NodePosY = Y;
		BreakCreator.Finalize();

		UEdGraphPin* VariablePin = GetNode->GetValuePin();
		UEdGraphPin* BreakInPin = FindFirstPin(BreakNode, EGPD_Input, UEdGraphSchema_K2::PC_Struct);
		UEdGraphPin* MemberPin = BreakNode->FindPin(Member, EGPD_Output);
		if (!VariablePin || !BreakInPin || !MemberPin || !Schema->TryCreateConnection(VariablePin, BreakInPin))
		{
			OutError = FString::Printf(TEXT("could not wire %s.%s"), *Variable.ToString(), *Member.ToString());
			return false;
		}

		UEdGraphPin* BoolOut = MemberPin;
		if (bNegate)
		{
			FGraphNodeCreator<UK2Node_CallFunction> NotCreator(*Graph);
			UK2Node_CallFunction* NotNode = NotCreator.CreateNode(false);
			NotNode->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(
				GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Not_PreBool)));
			NotNode->NodePosX = X - 180;
			NotNode->NodePosY = Y;
			NotCreator.Finalize();

			UEdGraphPin* NotIn = NotNode->FindPin(TEXT("A"), EGPD_Input);
			UEdGraphPin* NotOut = NotNode->GetReturnValuePin();
			if (!NotIn || !NotOut || !Schema->TryCreateConnection(MemberPin, NotIn))
			{
				OutError = TEXT("could not wire NOT node");
				return false;
			}
			BoolOut = NotOut;
		}

		if (!Schema->TryCreateConnection(BoolOut, ResultPin))
		{
			OutError = TEXT("could not connect to result");
			return false;
		}
		return true;
	}

	static void Run(const TArray<FString>& Args)
	{
		const bool bApply = Args.Contains(TEXT("apply"));

		UAnimBlueprint* AnimBP = LoadObject<UAnimBlueprint>(nullptr, AnimBPPath);
		if (!AnimBP)
		{
			UE_LOG(LogALSRewire, Error, TEXT("Could not load %s"), AnimBPPath);
			return;
		}

		// All transition and conduit rule graphs, by path relative to the anim BP
		TMap<FString, UAnimationTransitionGraph*> GraphsByPath;
		ForEachObjectWithOuter(AnimBP, [&GraphsByPath](UObject* Object)
		{
			if (UAnimationTransitionGraph* Graph = Cast<UAnimationTransitionGraph>(Object))
			{
				GraphsByPath.Add(RelativeGraphPath(Graph), Graph);
			}
		}, true);

		// Graphs copied from another state machine can keep their original object outer, so their
		// object path differs from the one shown in the editor. Also index every graph by its
		// hierarchy path (graph.node.subgraph...), which is what the entry table uses.
		{
			TArray<UEdGraph*> RootGraphs;
			RootGraphs.Append(AnimBP->FunctionGraphs);
			RootGraphs.Append(AnimBP->UbergraphPages);
			TArray<TPair<FString, UEdGraph*>> Stack;
			for (UEdGraph* Root : RootGraphs)
			{
				if (Root)
				{
					Stack.Emplace(Root->GetName(), Root);
				}
			}
			TSet<UEdGraph*> Visited;
			while (Stack.Num() > 0)
			{
				const TPair<FString, UEdGraph*> Current = Stack.Pop();
				if (Visited.Contains(Current.Value))
				{
					continue;
				}
				Visited.Add(Current.Value);
				if (UAnimationTransitionGraph* TransitionGraph = Cast<UAnimationTransitionGraph>(Current.Value))
				{
					GraphsByPath.FindOrAdd(Current.Key, TransitionGraph);
				}
				for (UEdGraphNode* Node : Current.Value->Nodes)
				{
					if (!Node)
					{
						continue;
					}
					for (UEdGraph* SubGraph : Node->GetSubGraphs())
					{
						if (SubGraph)
						{
							Stack.Emplace(Current.Key + TEXT(".") + Node->GetName() + TEXT(".") + SubGraph->GetName(), SubGraph);
						}
					}
				}
			}
		}

		TOptional<FScopedTransaction> Transaction;
		if (bApply)
		{
			Transaction.Emplace(LOCTEXT("RewireTransitionRules", "Rewire ALS transition rules"));
			AnimBP->Modify();
		}

		int32 Done = 0;
		int32 Failed = 0;
		for (const FEntry& Entry : Entries)
		{
			UAnimationTransitionGraph* const* GraphPtr = GraphsByPath.Find(Entry.Graph);
			UAnimationTransitionGraph* Graph = GraphPtr ? *GraphPtr : nullptr;
			if (!Graph)
			{
				// Fall back to ignoring state machine node numbers, only when exactly one graph matches
				const FString Wanted = NormalizeMachineNodeNames(Entry.Graph);
				TSet<UAnimationTransitionGraph*> Candidates;
				for (const TPair<FString, UAnimationTransitionGraph*>& Pair : GraphsByPath)
				{
					if (NormalizeMachineNodeNames(Pair.Key) == Wanted)
					{
						Candidates.Add(Pair.Value);
					}
				}
				if (Candidates.Num() == 1)
				{
					Graph = *Candidates.CreateConstIterator();
				}
				else if (Candidates.Num() > 1)
				{
					UE_LOG(LogALSRewire, Warning, TEXT("AMBIGUOUS %s (%d matches)"), Entry.Graph, Candidates.Num());
				}
			}
			UAnimGraphNode_TransitionResult* Result = Graph ? Graph->GetResultNode() : nullptr;
			if (!Result)
			{
				UE_LOG(LogALSRewire, Warning, TEXT("MISSING  %s"), Entry.Graph);
				++Failed;
				continue;
			}

			FString Error;
			if (Entry.Action == EAction::AutoRule)
			{
				UAnimStateTransitionNode* TransitionNode = Cast<UAnimStateTransitionNode>(Graph->GetOuter());
				if (!TransitionNode)
				{
					UE_LOG(LogALSRewire, Warning, TEXT("FAILED   %s: not a transition graph"), Entry.Graph);
					++Failed;
					continue;
				}
				UE_LOG(LogALSRewire, Display, TEXT("%s %s -> automatic rule"), bApply ? TEXT("REWIRED ") : TEXT("WOULD   "), Entry.Graph);
				if (bApply)
				{
					TransitionNode->Modify();
					Graph->Modify();
					TransitionNode->bAutomaticRuleBasedOnSequencePlayerInState = true;
					TransitionNode->AutomaticRuleTriggerTime = 0.0f;
					ClearGraph(AnimBP, Graph, Result);
				}
				++Done;
				continue;
			}

			FName Variable;
			UScriptStruct* Struct = nullptr;
			if (!ResolveSource(Entry, Variable, Struct, Error))
			{
				UE_LOG(LogALSRewire, Warning, TEXT("FAILED   %s: %s"), Entry.Graph, *Error);
				++Failed;
				continue;
			}

			UE_LOG(LogALSRewire, Display, TEXT("%s %s -> %s%s.%s"), bApply ? TEXT("REWIRED ") : TEXT("WOULD   "),
			       Entry.Graph, Entry.bNegate ? TEXT("NOT ") : TEXT(""), *Variable.ToString(), Entry.Member);
			if (bApply)
			{
				Graph->Modify();
				ClearGraph(AnimBP, Graph, Result);
				if (!WireRule(Graph, Result, Variable, Struct, FName(Entry.Member), Entry.bNegate, Error))
				{
					UE_LOG(LogALSRewire, Error, TEXT("FAILED   %s: %s (graph left with result = false)"), Entry.Graph, *Error);
					++Failed;
					continue;
				}
			}
			++Done;
		}

		if (bApply)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
			FKismetEditorUtilities::CompileBlueprint(AnimBP);
			// Compiling can leave the package clean; make sure the editor offers to save the changes
			AnimBP->MarkPackageDirty();
		}

		UE_LOG(LogALSRewire, Display, TEXT("%s: %d ok, %d failed, %d total. %s"),
		       bApply ? TEXT("Rewired") : TEXT("Dry run"), Done, Failed, UE_ARRAY_COUNT(Entries),
		       bApply ? TEXT("Anim BP compiled, not saved.") : TEXT("Run with 'apply' to change the anim BP."));
	}
}

static FAutoConsoleCommand GRewireTransitionRulesCommand(
	TEXT("adb.Anim.RewireTransitionRules"),
	TEXT("Rewires ALS_AnimBP transition rules to read UALSCharacterAnimInstance::TransitionRules. "
		"Without 'apply' it only lists what it would change. Usage: adb.Anim.RewireTransitionRules [apply]"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&ALSRewire::Run));

IMPLEMENT_MODULE(FDefaultModuleImpl, ALSV4_CPPEditor)

#undef LOCTEXT_NAMESPACE
