//#pragma once
//
//#include "d3dx12.h"
//#include "wrl.h"
//#include <unordered_map>
//
//struct
//{
//    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
//};
//
//class ResourceStateTracker
//{
//public:
//
//    void RemoveResource(Microsoft::WRL::ComPtr<ID3D12Resource> a_Resource);
//    void UpdateResourceState(Microsoft::WRL::ComPtr<ID3D12Resource> a_Resource, D3D12_RESOURCE_STATES a_NewState);
//    D3D12_RESOURCE_STATES GetStateOfResource(Microsoft::WRL::ComPtr<ID3D12Resource> a_Resource);
//private:
//    std::unordered_map<Microsoft::WRL::ComPtr<ID3D12Resource>, D3D12_RESOURCE_STATES> m_TrackedStates;
//
//};
//
