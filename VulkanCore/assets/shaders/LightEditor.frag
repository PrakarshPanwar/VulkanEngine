#version 460 core

layout(location = 0) in flat int v_InstanceID;

layout(location = 0) out int o_EntityID;

void main()
{
	o_EntityID = v_InstanceID;
}