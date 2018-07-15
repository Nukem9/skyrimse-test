#pragma once

#include "NiAVObject.h"
#include "NiCullingProcess.h"

#include "../MOC.h"
extern bool doCullTest;

class BSCullingProcess : public NiCullingProcess
{
public:
	char _pad2[0x30070];
	int kCullMode;
	BSCompoundFrustum *pCompoundFrustum;
	char _pad3[0x2C];
	bool m_bDontUseVirtualAppend;

	virtual ~BSCullingProcess();
	virtual void Process(NiAVObject *Object, uint32_t Unknown) override;
	virtual void Process(const NiCamera *Camera, NiAVObject *Object, class NiVisibleArray *Array) override;
	virtual void AppendVirtual(BSGeometry *Geometry, uint32_t Unknown) override;
	virtual void AppendNonAccum(NiAVObject *Object);
	virtual bool TestBaseVisibility1(class BSMultiBound& Bound);	// MSVC doesn't order these correctly without numbers
	virtual bool TestBaseVisibility2(class BSOcclusionPlane& Bound);// ^
	virtual bool TestBaseVisibility3(NiBound& Bound);				// ^

	void hk_Process(NiAVObject *Object, uint32_t Unknown)
	{
		if (!Object->IsVisualObjectI() && !Object->QAlwaysDraw())
			return;

		auto appendOrAccumulate = [this, Object, Unknown]()-> void
		{
			if (!m_bDontUseVirtualAppend)
			{
				AppendVirtual(static_cast<BSGeometry *>(Object), Unknown);
				return;
			}

			Object->OnVisible(this, Unknown);
			SetAccumulated(Object, true);
		};

		// ALLPASS
		if (kCullMode == 1)
		{
			// Don't cull anything
			return appendOrAccumulate();
		}

		// ALLFAIL
		if (kCullMode == 2)
		{
			if (m_bDontUseVirtualAppend)
				SetAccumulated(Object, false);

			return;
		}

		/*if (doCullTest && !MOC::TestSphere(Object))
		{
		if (m_bDontUseVirtualAppend)
		SetAccumulated(Object, false);

		return;
		}*/

		// NORMAL, IGNOREMULTIBOUNDS, FORCEMULTIBOUNDSNOUPDATE
		if (!Object->QPreProcessedNode() || bIgnorePreprocess || kCullMode == 4)
		{
			// Compound frustum plane testing (frustum + world culling planes)
			if (pCompoundFrustum && pCompoundFrustum->dwordBC && kCullMode != 3)
			{
				if (Object->QAlwaysDraw())
					return appendOrAccumulate();

				int compoundPlanes[256];// Array size might not be accurate (STACK SMASH WARNING)
				uint32_t frustumPlanes;

				pCompoundFrustum->GetActivePlaneState(compoundPlanes);
				frustumPlanes = m_kPlanes.GetActivePlaneState();

				bool primaryVisCheck = (pCompoundFrustum->byteC4 || TestBaseVisibility3(Object->m_kWorldBound)) && pCompoundFrustum->Process(Object);

				if (primaryVisCheck && doCullTest)
					primaryVisCheck = MOC::TestSphere(Object);

				if (primaryVisCheck)
				{
					// Passed checks
					appendOrAccumulate();
				}
				else if (m_bDontUseVirtualAppend)
				{
					// Not visible
					SetAccumulated(Object, false);
				}

				pCompoundFrustum->SetActivePlaneState(compoundPlanes);
				m_kPlanes.SetActivePlaneState(frustumPlanes);
			}
			else
			{
				// Frustum culling only
				if (m_bDontUseVirtualAppend)
				{
					if (doCullTest && !MOC::TestSphere(Object))
						return;

					if (!Object->QAlwaysDraw() && m_kPlanes.IsAnyPlaneActive())
					{
						// Test each plane
						DoCulling(Object, Unknown);
						return;
					}

					Object->OnVisible(this, Unknown);
					SetAccumulated(Object, true);
					return;
				}

				if (Object->QAlwaysDraw() || !m_kPlanes.IsAnyPlaneActive() || TestBaseVisibility3(Object->m_kWorldBound))
				{
					if (doCullTest && !MOC::TestSphere(Object))
						return;

					AppendVirtual(static_cast<BSGeometry *>(Object), Unknown);
				}
			}
		}
		else
		{
			if (!Object->QNotVisible())
				return appendOrAccumulate();

			if (m_bDontUseVirtualAppend)
				SetAccumulated(Object, false);
		}
	}
};
static_assert_offset(BSCullingProcess, kCullMode, 0x30198);
static_assert_offset(BSCullingProcess, pCompoundFrustum, 0x301A0);
static_assert_offset(BSCullingProcess, m_bDontUseVirtualAppend, 0x301D4);

STATIC_CONSTRUCTOR(CheckBSCullingProcess, []
{
	assert_vtable_index(&BSCullingProcess::AppendNonAccum, 25);
	assert_vtable_index(&BSCullingProcess::TestBaseVisibility1, 26);
	assert_vtable_index(&BSCullingProcess::TestBaseVisibility2, 27);
	assert_vtable_index(&BSCullingProcess::TestBaseVisibility3, 28);
});