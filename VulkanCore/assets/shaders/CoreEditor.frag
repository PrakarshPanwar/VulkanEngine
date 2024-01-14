#version 460 core

layout(location = 0) out int o_EntityID;

layout(location = 0) in flat int o_InstanceID;

void main()
{
    o_EntityID = o_InstanceID;
}