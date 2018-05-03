#pragma once

class BaseFormComponent;
class TESForm;
class TESObjectREFR;
class TESObjectCell;

#define TES_FORM_MASTER_COUNT	256			// Maximum master file index + 1 (2^8, 8 bits)
#define TES_FORM_INDEX_COUNT	16777216	// Maximum index + 1 (2^24, 24 bits)

class BaseFormComponent
{
public:
	virtual ~BaseFormComponent();
	virtual void VFunc01() = 0;
	virtual void VFunc02() = 0;
	virtual void VFunc03();
};

class TESForm : public BaseFormComponent
{
public:
	virtual ~TESForm();
	virtual void VFunc04();
	virtual void VFunc05();
	virtual void VFunc06();
	virtual void VFunc07();
	virtual void VFunc08();
	virtual void VFunc09();
	virtual void VFunc10();
	virtual void VFunc11();
	virtual void VFunc12();
	virtual void VFunc13();
	virtual void VFunc14();
	virtual void VFunc15();
	virtual void VFunc16();
	virtual void VFunc17();
	virtual void VFunc18();
	virtual void VFunc19();
	virtual void VFunc20();
	virtual void VFunc21();
	virtual void VFunc22();
	virtual void VFunc23();
	virtual void VFunc24();
	virtual void VFunc25();
	virtual void VFunc26();
	virtual void VFunc27();
	virtual void VFunc28();
	virtual void VFunc29();
	virtual void VFunc30();
	virtual void VFunc31();
	virtual void VFunc32();
	virtual void VFunc33();
	virtual void VFunc34();
	virtual void VFunc35();
	virtual void VFunc36();
	virtual void VFunc37();
	virtual void VFunc38();
	virtual void VFunc39();
	virtual void VFunc40();
	virtual void VFunc41();
	virtual void VFunc42();
	virtual void VFunc43();
	virtual TESObjectREFR *IsREFR();
	virtual void VFunc45();
	virtual void VFunc46();
	virtual void VFunc47();
	virtual void VFunc48();
	virtual void VFunc49();
	virtual void VFunc50();
	virtual void VFunc51();
	virtual void VFunc52();
	virtual void VFunc53();
	virtual void VFunc54();
	virtual void VFunc55();
	virtual void VFunc56();
	virtual void VFunc57();
	virtual void VFunc58();
	virtual void VFunc59();

	static TESForm *LookupFormById(uint32_t FormId);
};

class NiNode;

class TESObjectREFR : public TESForm/*, BSHandleRefObject, BSTEventSink<struct BSAnimationGraphEvent>, IAnimationGraphManagerHolder */
{
public:
	virtual ~TESObjectREFR();
	virtual void VFunc60();
	virtual void VFunc61();
	virtual void VFunc62();
	virtual void VFunc63();
	virtual void VFunc64();
	virtual void VFunc65();
	virtual void VFunc66();
	virtual void VFunc67();
	virtual void VFunc68();
	virtual void VFunc69();
	virtual void VFunc70();
	virtual void VFunc71();
	virtual void VFunc72();
	virtual void VFunc73();
	virtual void VFunc74();
	virtual void VFunc75();
	virtual void VFunc76();
	virtual void VFunc77();
	virtual void VFunc78();
	virtual void VFunc79();
	virtual void VFunc80();
	virtual void VFunc81();
	virtual void VFunc82();
	virtual void VFunc83();
	virtual void VFunc84();
	virtual void VFunc85();
	virtual void VFunc86();
	virtual void VFunc87();
	virtual void VFunc88();
	virtual void VFunc89();
	virtual void VFunc90();
	virtual void VFunc91();
	virtual void VFunc92();
	virtual void VFunc93();
	virtual void VFunc94();
	virtual void VFunc95();
	virtual void VFunc96();
	virtual void VFunc97();
	virtual void VFunc98();
	virtual void VFunc99();
	virtual void VFunc100();
	virtual void VFunc101();
	virtual void VFunc102();
	virtual void VFunc103();
	virtual void VFunc104();
	virtual void VFunc105();
	virtual void VFunc106();
	virtual void VFunc107();
	virtual void VFunc108();
	virtual void VFunc109();
	virtual void VFunc110();
	virtual void VFunc111();
	virtual NiNode *GetNiNode();

	TESForm *GetBaseObject() const
	{
		return *(TESForm **)((__int64)this + 0x40);
	}

	TESObjectCell *GetParentCell() const
	{
		return *(TESObjectCell **)((__int64)this + 0x60);
	}
};

class TESObjectCell : public TESForm/*, public TESFullName */
{
public:
	bool IsAttached() const
	{
		return *(uint8_t *)((__int64)this + 0x44) == 7;
	}
};

STATIC_CONSTRUCTOR(CheckTESForm, []
{
	assert_vtable_index(&BaseFormComponent::~BaseFormComponent, 0);
	assert_vtable_index(&BaseFormComponent::VFunc01, 1);
	assert_vtable_index(&BaseFormComponent::VFunc02, 2);
	assert_vtable_index(&BaseFormComponent::VFunc03, 3);

	assert_vtable_index(&TESForm::~TESForm, 0);
	assert_vtable_index(&TESForm::VFunc04, 4);
	assert_vtable_index(&TESForm::IsREFR, 44);
	assert_vtable_index(&TESForm::VFunc59, 59);

	assert_vtable_index(&TESObjectREFR::~TESObjectREFR, 0);
	assert_vtable_index(&TESObjectREFR::VFunc60, 60);
	assert_vtable_index(&TESObjectREFR::VFunc100, 100);
	assert_vtable_index(&TESObjectREFR::GetNiNode, 112);
});