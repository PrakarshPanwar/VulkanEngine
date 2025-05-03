#version 460

layout(location = 0) out int o_EntityID;

layout(location = 0) in flat int v_InstanceID;

void main()
{
    o_EntityID = v_InstanceID;
}