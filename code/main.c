// LICENSE AT END OF FILE (MIT).


#include "third_party/third_party_inc.h"
#include "third_party/third_party_inc.cpp"

#define internal static
#define global static


global MD_Arena *sg_arena = 0;


//~ NOTE(fakhri): Types

typedef MD_u32 TypeInfoKind;
enum
{
    TypeInfoKind_Base,
    TypeInfoKind_Struct,
    TypeInfoKind_Enum,
    TypeInfoKind_Union,
};


typedef struct FieldInfo FieldInfo;
struct FieldInfo
{
    FieldInfo *next;
    
    MD_String8 name;
    struct TypeInfo *type;
    MD_b32 is_array;
    MD_String8 array_len;
};

typedef struct TypeInfo TypeInfo;
struct TypeInfo
{
    // NOTE(fakhri): for linked list use
    TypeInfo *next;
    TypeInfoKind kind;
    MD_Node *node;
    MD_String8 name;
    MD_String8 macro_alias;
    MD_String8 typedef_alias;
    MD_String8 serialize_function;
    MD_String8 deserialize_function;
    // NOTE(fakhri): base type of enums
    TypeInfo *base_type;
    // NOTE(fakhri): discriminated union stuff
    TypeInfo *union_kind_enum_type;
    // NOTE(fakhri): struct variants linked list
    TypeInfo *first_struct_variant;
    TypeInfo *last_struct_variant;
    // NOTE(fakhri): fields linked list
    FieldInfo *first_field;
    FieldInfo *last_field;
};


//~

internal void
SG_PushMessageList(MD_Arena *arena, MD_MessageList *list, MD_MessageKind kind, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    MD_String8 msg_text = MD_S8FmtV(arena, fmt, args);
    va_end(args);
    
    MD_Message* msg = MD_MakeNodeError(arena, 0, MD_MessageKind_Error, msg_text);
    MD_MessageListPush(list, msg);
}

internal TypeInfo *
GetTypeInfoFromMap_ByStr(MD_Map *map, MD_String8 string)
{
    TypeInfo *result = 0;
    MD_MapKey key = MD_MapKeyStr(string);
    MD_MapSlot *slot = MD_MapLookup(map, key);
    if (slot)
    {
        result = (TypeInfo *)slot->val;
    }
    return result;
}

internal void
ProcessStructFieldNode(MD_Arena *arena, MD_Map *types_map, TypeInfo *type_info, MD_Node *field_node, MD_MessageList *message_list)
{
    FieldInfo *field_info = MD_PushArrayZero(arena, FieldInfo, 1);
    field_info->name = MD_S8Copy(arena, field_node->string);
    MD_Node *field_type_node = field_node->first_child;
    if (!MD_NodeIsNil(field_type_node))
    {
        field_info->type = GetTypeInfoFromMap_ByStr(types_map, field_type_node->string);
        if (field_info->type)
        {
            if (MD_NodeHasTag(field_type_node, MD_S8Lit("array"), 0))
            {
                field_info->is_array = 1;
                MD_Node *array_tag = MD_TagFromString(field_type_node, MD_S8Lit("array"), 0);
                MD_Node *len_node = MD_FirstNodeWithString(array_tag->first_child, MD_S8Lit("len"), 0);
                if (!MD_NodeIsNil(len_node))
                {
                    field_info->array_len = MD_S8Copy(arena, len_node->string);
                }
                else
                {
                    // NOTE(fakhri): missing len node
                    SG_PushMessageList(arena, message_list, MD_MessageKind_Error, 
                                       "array types require len to be specified, add @array(len: YOUR_LEN_HERE)", MD_S8VArg(field_type_node->string));
                }
            }
            MD_QueuePush(type_info->first_field, type_info->last_field, field_info);
        }
        else
        {
            // NOTE(fakhri): couldn't find type
            SG_PushMessageList(arena, message_list, MD_MessageKind_Error, 
                               "Type %.*s is not declared anywhere in file", MD_S8VArg(field_type_node->string));
        }
    }
    else
    {
        // NOTE(fakhri): struct field with no type
        SG_PushMessageList(arena, message_list, MD_MessageKind_Error, 
                           "Missing field type in struct %.*s, for field %.*s", 
                           MD_S8VArg(type_info->name), MD_S8VArg(field_node->string));
    }
}

internal MD_MessageList
ProcessParsedResult(MD_Arena *arena, MD_Node *node, MD_Map *types_map)
{
    MD_MessageList message_list = MD_ZERO_STRUCT;
    // NOTE(fakhri): higher information gathering about types
    for(MD_EachNode(type_node, node->first_child))
    {
        TypeInfo *type_info = GetTypeInfoFromMap_ByStr(types_map, type_node->string);
        if (type_info == 0)
        {
            type_info = MD_PushArrayZero(arena, TypeInfo, 1);
            MD_MapInsert(arena, types_map, MD_MapKeyStr(type_node->string), type_info);
            type_info->node = type_node;
            type_info->name = MD_S8Copy(arena, type_node->string);
            if (MD_NodeHasTag(type_node, MD_S8Lit("base"), 0))
            {
                type_info->kind = TypeInfoKind_Base;
                type_info->macro_alias = MD_S8Fmt(arena, "SG_%.*s_TYPE", MD_S8VArg(type_node->string));
                type_info->typedef_alias = MD_S8Fmt(arena, "SG_%.*s", MD_S8VArg(type_node->string));
            }
            else if (MD_NodeHasTag(type_node, MD_S8Lit("struct"), 0))
            {
                type_info->kind = TypeInfoKind_Struct;
            }
            else if (MD_NodeHasTag(type_node, MD_S8Lit("enum"), 0))
            {
                type_info->kind = TypeInfoKind_Enum;
            }
            else if (MD_NodeHasTag(type_node, MD_S8Lit("union"), 0))
            {
                type_info->kind = TypeInfoKind_Union;
            }
            else
            {
                // NOTE(fakhri): unkown tag
                SG_PushMessageList(arena, &message_list, MD_MessageKind_Warning, 
                                   "node %.*s of unkown type", MD_S8VArg(type_node->string));
            }
            
            type_info->serialize_function   = MD_S8Fmt(arena, "SG_Serialize_%.*s", MD_S8VArg(type_node->string));
            type_info->deserialize_function = MD_S8Fmt(arena, "SG_Deserialize_%.*s", MD_S8VArg(type_node->string));
        }
        else
        {
            // NOTE(fakhri): Redefinition of type
            SG_PushMessageList(arena, &message_list, MD_MessageKind_Error, 
                               "Type %.*s defined mutliple times", MD_S8VArg(type_node->string));
        }
    }
    
    // NOTE(fakhri): gathering fields information
    for(MD_EachNode(type_node, node->first_child))
    {
        TypeInfo *type_info = GetTypeInfoFromMap_ByStr(types_map, type_node->string);
        
        switch(type_info->kind)
        {
            case TypeInfoKind_Struct:
            {
                for (MD_EachNode(field_node, type_info->node->first_child))
                {
                    ProcessStructFieldNode(arena, types_map, type_info, field_node, &message_list);
                }
            } break;
            case TypeInfoKind_Enum:
            {
                MD_Node *tag_enum = MD_TagFromString(type_node, MD_S8Lit("enum"), 0);
                MD_Node *tage_base_type = MD_ChildFromString(tag_enum, MD_S8Lit("base_type"), 0);
                if (!MD_NodeIsNil(tage_base_type) && !MD_NodeIsNil(tage_base_type->first_child))
                {
                    type_info->base_type = GetTypeInfoFromMap_ByStr(types_map, 
                                                                    tage_base_type->first_child->string);
                    if (type_info->base_type)
                    {
                        MD_Assert(type_info->base_type);
                        for (MD_EachNode(field_node, type_info->node->first_child))
                        {
                            FieldInfo *field_info = MD_PushArrayZero(arena, FieldInfo, 1);
                            field_info->name = MD_S8Copy(arena, field_node->string);
                            MD_QueuePush(type_info->first_field, type_info->last_field, field_info);
                        }
                    }
                    else
                    {
                        // NOTE(fakhri): couldn't find type
                        SG_PushMessageList(arena, &message_list, MD_MessageKind_Error, 
                                           "Type %.*s is not declared anywhere in file", MD_S8VArg(tage_base_type->first_child->string));
                    }
                }
                else
                {
                    // NOTE(fakhri): base_type not present
                    SG_PushMessageList(arena, &message_list, MD_MessageKind_Error, 
                                       "Missing base type in enum type %.*s", MD_S8VArg(type_info->name));
                }
                
            } break;
            case TypeInfoKind_Union:
            {
                MD_Node *tag_union = MD_TagFromString(type_node, MD_S8Lit("union"), 0);
                MD_Node *tag_base_type = MD_ChildFromString(tag_union, MD_S8Lit("base_type"), 0);
                if (!MD_NodeIsNil(tag_base_type) && !MD_NodeIsNil(tag_base_type->first_child))
                {
                    // NOTE(fakhri): generate kind enum type
                    TypeInfo *union_kind_enum_type = MD_PushArrayZero(arena, TypeInfo, 1);
                    union_kind_enum_type->kind = TypeInfoKind_Enum;
                    union_kind_enum_type->name = MD_S8Fmt(arena, "%.*sKind", MD_S8VArg(type_info->name));
                    union_kind_enum_type->base_type = GetTypeInfoFromMap_ByStr(types_map, 
                                                                               tag_base_type->first_child->string);
                    if (union_kind_enum_type->base_type)
                    {
                        union_kind_enum_type->serialize_function = MD_S8Fmt(arena, "SG_Serialize_%.*s", MD_S8VArg(union_kind_enum_type->name));
                        union_kind_enum_type->deserialize_function = MD_S8Fmt(arena, "SG_Deserialize_%.*s", MD_S8VArg(union_kind_enum_type->name));
                        
                        type_info->union_kind_enum_type = union_kind_enum_type;
                        
                        // NOTE(fakhri): add enumerate fields to kind enum
                        for (MD_EachNode(field_node, type_info->node->first_child))
                        {
                            FieldInfo *field_info = MD_PushArrayZero(arena, FieldInfo, 1);
                            field_info->name = MD_S8Fmt(arena, "%.*s_%.*s", MD_S8VArg(union_kind_enum_type->name), MD_S8VArg(field_node->string));
                            MD_QueuePush(union_kind_enum_type->first_field, union_kind_enum_type->last_field, field_info);
                        }
                        
                        // NOTE(fakhri): kind field
                        {
                            FieldInfo *kind_field = MD_PushArrayZero(arena, FieldInfo, 1);
                            kind_field->name = MD_S8Lit("kind");
                            kind_field->type = union_kind_enum_type;
                            MD_QueuePush(type_info->first_field, type_info->last_field, kind_field);
                        }
                        
                        // NOTE(fakhri): generate struct variants
                        for (MD_EachNode(variant_node, type_info->node->first_child))
                        {
                            TypeInfo *struct_variant = MD_PushArrayZero(arena, TypeInfo, 1);
                            struct_variant->kind = TypeInfoKind_Struct;
                            struct_variant->name = MD_S8Fmt(arena, "%.*s_%.*s", MD_S8VArg(type_info->name), MD_S8VArg(variant_node->string));
                            struct_variant->serialize_function = MD_S8Fmt(arena, "SG_Serialize_%.*s", MD_S8VArg(struct_variant->name));
                            struct_variant->deserialize_function = MD_S8Fmt(arena, "SG_Deserialize_%.*s", MD_S8VArg(struct_variant->name));
                            
                            // NOTE(fakhri): add a field for the variant
                            {
                                FieldInfo *struct_vairant_field = MD_PushArrayZero(arena, FieldInfo, 1);
                                struct_vairant_field->name = MD_S8Stylize(arena, variant_node->string, MD_IdentifierStyle_LowerCase, MD_S8Lit("_"));
                                struct_vairant_field->type = struct_variant;
                                MD_QueuePush(type_info->first_field, type_info->last_field, struct_vairant_field);
                            }
                            
                            for (MD_EachNode(field_type_node, variant_node->first_child))
                            {
                                ProcessStructFieldNode(arena,types_map, struct_variant, field_type_node, &message_list);
                            }
                            
                            MD_QueuePush(type_info->first_struct_variant, type_info->last_struct_variant, struct_variant);
                        }
                    }
                    else
                    {
                        // NOTE(fakhri): couldn't find type
                        SG_PushMessageList(arena, &message_list, MD_MessageKind_Error, 
                                           "Type %.*s is not declared anywhere in file", MD_S8VArg(tag_base_type->first_child->string));
                    }
                }
                else
                {
                    // NOTE(fakhri): base_type not present
                    SG_PushMessageList(arena, &message_list, MD_MessageKind_Error, 
                                       "Missing base type in union type %.*s", MD_S8VArg(type_info->name));
                }
            } break;
        }
    }
    
    return message_list;
}

internal void
PushTypedefNameIfBaseElseNormalName(MD_Arena *arena, TypeInfo *type_info, MD_String8List *stream)
{
    if (type_info->kind == TypeInfoKind_Base)
    {
        MD_S8ListPush(arena, stream, type_info->typedef_alias);
    }
    else
    {
        MD_S8ListPush(arena, stream, type_info->name);
    }
}

internal void
PushTypedef_Enum(MD_Arena *arena, TypeInfo *type_info, MD_String8List *stream)
{
    MD_S8ListPush(arena, stream, MD_S8Lit("typedef "));
    MD_S8ListPush(arena, stream, type_info->base_type->typedef_alias);
    MD_S8ListPush(arena, stream, MD_S8Lit(" "));
    MD_S8ListPush(arena, stream, type_info->name);
    MD_S8ListPush(arena, stream, MD_S8Lit(";\n"));
}

internal void
PushTypedef_Struct(MD_Arena *arena, TypeInfo *type_info, MD_String8List *stream)
{
    MD_S8ListPush(arena, stream, MD_S8Lit("typedef struct "));
    MD_S8ListPush(arena, stream, type_info->name);
    MD_S8ListPush(arena, stream, MD_S8Lit(" "));
    MD_S8ListPush(arena, stream, type_info->name);
    MD_S8ListPush(arena, stream, MD_S8Lit(";\n"));
}

internal void
PushTypeDeclaration_Enum(MD_Arena *arena, TypeInfo *type_info, MD_String8List *stream)
{
    MD_S8ListPush(arena, stream, MD_S8Lit("enum\n{\n"));
    
    for (FieldInfo *field_info = type_info->first_field;
         field_info != 0;
         field_info = field_info->next)
    {
        MD_S8ListPush(arena, stream, MD_S8Lit("\t"));
        MD_S8ListPush(arena, stream, field_info->name);
        MD_S8ListPush(arena, stream, MD_S8Lit(",\n"));
    }
    MD_S8ListPush(arena, stream, MD_S8Lit("};\n"));
}

internal void
PushTypeDeclaration_Struct(MD_Arena *arena, TypeInfo *type_info, MD_String8List *stream)
{
    MD_S8ListPush(arena, stream, MD_S8Lit("struct "));
    MD_S8ListPush(arena, stream, type_info->name);
    MD_S8ListPush(arena, stream, MD_S8Lit("\n{\n"));
    
    for (FieldInfo *field_info = type_info->first_field;
         field_info != 0;
         field_info = field_info->next)
    {
        MD_S8ListPush(arena, stream, MD_S8Lit("\t"));
        PushTypedefNameIfBaseElseNormalName(arena, field_info->type, stream);
        MD_S8ListPush(arena, stream, MD_S8Lit(" "));
        MD_S8ListPush(arena, stream, field_info->name);
        if (field_info->is_array)
        {
            MD_S8ListPush(arena, stream, MD_S8Lit("["));
            MD_S8ListPush(arena, stream, field_info->array_len);
            MD_S8ListPush(arena, stream, MD_S8Lit("]"));
        }
        MD_S8ListPush(arena, stream, MD_S8Lit(";\n"));
    }
    MD_S8ListPush(arena, stream, MD_S8Lit("};\n"));
}

internal void
PushFunctionDeclaration(MD_Arena *arena, TypeInfo *type_info, MD_String8List *stream, MD_String8 function_name)
{
    MD_S8ListPush(arena, stream, MD_S8Lit("static void "));
    MD_S8ListPush(arena, stream, function_name);
    MD_S8ListPush(arena, stream, MD_S8Lit("(SG_Buffer *buffer, "));
    PushTypedefNameIfBaseElseNormalName(arena, type_info, stream);
    MD_S8ListPush(arena, stream, MD_S8Lit(" *s);\n"));
}

internal void
PushFunctionSerializationCall(MD_Arena *arena, FieldInfo *field_info, MD_String8List *stream)
{
    MD_S8ListPush(arena, stream, field_info->type->serialize_function);
    MD_S8ListPush(arena, stream, MD_S8Lit("(buffer, "));
    MD_S8ListPush(arena, stream, MD_S8Lit("&s->"));
    MD_S8ListPush(arena, stream, field_info->name);
    MD_S8ListPush(arena, stream, MD_S8Lit(");"));
}

internal void
PushFunctionDeserializationCall(MD_Arena *arena, FieldInfo *field_info, MD_String8List *stream)
{
    MD_S8ListPush(arena, stream, field_info->type->deserialize_function);
    MD_S8ListPush(arena, stream, MD_S8Lit("(buffer, "));
    MD_S8ListPush(arena, stream, MD_S8Lit("&s->"));
    MD_S8ListPush(arena, stream, field_info->name);
    MD_S8ListPush(arena, stream, MD_S8Lit(");"));
}

internal void
PushFunctionDefinition_Enum(MD_Arena *arena, TypeInfo *type_info, MD_String8List *stream, MD_b32 serialize)
{
    MD_S8ListPush(arena, stream, MD_S8Lit("\t"));
    if (serialize)
    {
        MD_S8ListPush(arena, stream, type_info->base_type->serialize_function);
    }
    else
    {
        MD_S8ListPush(arena, stream, type_info->base_type->deserialize_function);
    }
    MD_S8ListPush(arena, stream, MD_S8Lit("(buffer, s);\n"));
}

internal void
PushFunctionDefinition_Struct(MD_Arena *arena, TypeInfo *type_info, MD_String8List *stream, MD_b32 serialize)
{
    for (FieldInfo *field_info = type_info->first_field;
         field_info != 0;
         field_info = field_info->next)
    {
        if (field_info->is_array)
        {
            MD_S8ListPush(arena, stream, MD_S8Lit("\tfor(int i = 0;\n"));
            MD_S8ListPush(arena, stream, MD_S8Lit("\t    i < SG_ArrayCount(s->"));
            MD_S8ListPush(arena, stream, field_info->name);
            MD_S8ListPush(arena, stream, MD_S8Lit(");\n\t    ++i)\n\t{\n"));
            
            MD_S8ListPush(arena, stream, MD_S8Lit("\t\t"));
            if (serialize)
            {
                MD_S8ListPush(arena, stream, field_info->type->serialize_function);
            }
            else
            {
                MD_S8ListPush(arena, stream, field_info->type->deserialize_function);
            }
            MD_S8ListPush(arena, stream, MD_S8Lit("(buffer, s->"));
            MD_S8ListPush(arena, stream, field_info->name);
            MD_S8ListPush(arena, stream, MD_S8Lit(" + i);\n"));
            
            MD_S8ListPush(arena, stream, MD_S8Lit("\t}\n"));
        }
        else
        {
            MD_S8ListPush(arena, stream, MD_S8Lit("\t"));
            if(serialize)
            {
                MD_S8ListPush(arena, stream, field_info->type->serialize_function);
            }
            else
            {
                MD_S8ListPush(arena, stream, field_info->type->deserialize_function);
            }
            MD_S8ListPush(arena, stream, MD_S8Lit("(buffer, &s->"));
            MD_S8ListPush(arena, stream, field_info->name);
            MD_S8ListPush(arena, stream, MD_S8Lit(");\n"));
        }
    }
}

internal void
PushFunctionSignatureDefinition(MD_Arena *arena, TypeInfo *type_info, MD_String8List *stream, MD_b32 serialize)
{
    MD_S8ListPush(arena, stream, MD_S8Lit("static void\n"));
    if(serialize)
    {
        MD_S8ListPush(arena, stream, type_info->serialize_function);
    }
    else
    {
        MD_S8ListPush(arena, stream, type_info->deserialize_function);
    }
    MD_S8ListPush(arena, stream, MD_S8Lit("(SG_Buffer *buffer, "));
    if (type_info->kind == TypeInfoKind_Base)
    {
        MD_S8ListPush(arena, stream, type_info->typedef_alias);
    }
    else
    {
        MD_S8ListPush(arena, stream, type_info->name);
    }
    
    MD_S8ListPush(arena, stream, MD_S8Lit(" *s)\n"));
    MD_S8ListPush(arena, stream, MD_S8Lit("{\n"));
}

//~ main //////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    // setup the global sg_arena
    sg_arena = MD_ArenaAlloc();
    
    MD_Assert(argc == 2);
    MD_String8 filename = MD_S8CString(argv[1]);
    MD_ParseResult parse_result = MD_ParseWholeFile(sg_arena, filename);
    
    // print metadesk errors
    for (MD_Message *message = parse_result.errors.first;
         message != 0;
         message = message->next)
    {
        MD_CodeLoc code_loc = MD_CodeLocFromNode(message->node);
        MD_PrintMessage(stdout, code_loc, message->kind, message->string);
    }
    
    // save to parse results list
    if (parse_result.errors.max_message_kind < MD_MessageKind_Error)
    {
        MD_Node *node = parse_result.node;
        MD_String8 folder = MD_PathChopLastSlash(filename);
        MD_String8 gen_folder = MD_S8Fmt(sg_arena, "%.*s/generated", MD_S8VArg(folder));
        MD_String8 gen_name = MD_PathChopLastPeriod(MD_PathSkipLastSlash(filename));
        MD_String8 h_filename = MD_S8Fmt(sg_arena, "%.*s/%.*s.meta.h", MD_S8VArg(gen_folder), MD_S8VArg(gen_name));
        MD_String8 c_filename = MD_S8Fmt(sg_arena, "%.*s/%.*s.meta.c", MD_S8VArg(gen_folder), MD_S8VArg(gen_name));
        FILE *h = fopen((char *)h_filename.str, "w");
        FILE *c = fopen((char *)c_filename.str, "w");
        
        if (h && c)
        {
            MD_Map types_map = MD_MapMake(sg_arena);
            MD_MessageList process_errors = ProcessParsedResult(sg_arena, node, &types_map);
            
            // print processing errors
            for (MD_Message *message = process_errors.first;
                 message != 0;
                 message = message->next)
            {
                MD_String8 kind_string = MD_StringFromMessageKind(message->kind);
                MD_String8 msg_out = MD_S8Fmt(sg_arena, "%.*s: %.*s\n", MD_S8VArg(kind_string), MD_S8VArg(message->string));
                fwrite(msg_out.str, msg_out.size, 1, stdout);
            }
            
            if (process_errors.max_message_kind < MD_MessageKind_Error)
            {
                // NOTE(fakhri): generating .h
                {
                    MD_String8List stream = {0};
                    
                    // NOTE(fakhri): include guards
                    MD_String8 upper_gen_name = MD_S8Stylize(sg_arena, gen_name, MD_IdentifierStyle_UpperCase, MD_S8Lit(""));
                    MD_String8 guard_name = MD_S8Fmt(sg_arena, "%.*s_META_H", MD_S8VArg(upper_gen_name));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#ifndef "));
                    MD_S8ListPush(sg_arena, &stream, guard_name);
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#define "));
                    MD_S8ListPush(sg_arena, &stream, guard_name);
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n\n"));
                    // NOTE(fakhri): helper defines
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#ifndef SG_ArrayCount\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("# define SG_ArrayCount(a) (sizeof(a) / sizeof((a)[0]))\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#endif\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#define SG_MAX(a, b) (((a)>(b))?(a):(b))\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#define SG_MIN(a, b) (((a)<(b))?(a):(b))\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#ifndef SG_ASSERT\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#define SG_ASSERT(c) if (!(c)) { *(volatile int *)0 = 0; }\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#endif\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    // NOTE(fakhri): buffer type
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#ifndef SG_BUFFER_TYPE\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("# error Buffer Type Was not Provided\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#endif\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    // NOTE(fakhri): buffer helper operation macros
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("typedef SG_BUFFER_TYPE SG_Buffer;\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#ifndef SG_WRITE_TO_BUFFER\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("# error Definition For SG_WRITE_TO_BUFFER(buffer, src, nbytes) was not provided\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#endif\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#ifndef SG_READ_FROM_BUFFER\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("# error Definition For SG_READ_FROM_BUFFER(buffer, dst, nbytes) was not provided\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#endif\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#ifndef SG_BUFFER_FREE_SIZE\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("# error Definition for SG_BUFFER_FREE_SIZE(buffer) was not provided\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#endif\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#ifndef SG_BUFFER_SIZE\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("# error Definition For SG_BUFFER_SIZE(buffer) was not provided\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#endif\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    
                    // NOTE(fakhri): base types defines
                    {
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("// Base Types\n"));
                        for(MD_EachNode(type_node, node->first_child))
                        {
                            TypeInfo *type_info = GetTypeInfoFromMap_ByStr(&types_map, type_node->string);
                            if (type_info->kind == TypeInfoKind_Base)
                            {
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#ifndef "));
                                MD_S8ListPush(sg_arena, &stream, type_info->macro_alias);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("# error Definition for "));
                                MD_S8ListPush(sg_arena, &stream, type_info->macro_alias);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit(" was not provided\n"));
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#endif\n"));
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("typedef "));
                                MD_S8ListPush(sg_arena, &stream, type_info->macro_alias);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit(" "));
                                MD_S8ListPush(sg_arena, &stream, type_info->typedef_alias);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit(";\n"));
                            }
                        }
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    }
                    
                    // NOTE(fakhri): generated types declaration
                    {
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("// Generated Types\n"));
                        for(MD_EachNode(type_node, node->first_child))
                        {
                            TypeInfo *type_info = GetTypeInfoFromMap_ByStr(&types_map, type_node->string);
                            switch(type_info->kind)
                            {
                                case TypeInfoKind_Enum:
                                {
                                    PushTypedef_Enum(sg_arena, type_info, &stream);
                                } break;
                                case TypeInfoKind_Union:
                                {
                                    // NOTE(fakhri): typedef the union as srtuct
                                    PushTypedef_Struct(sg_arena, type_info, &stream);
                                    
                                    // NOTE(fakhri): typedef union's enum kind
                                    PushTypedef_Enum(sg_arena, type_info->union_kind_enum_type, &stream);
                                    
                                    // NOTE(fakhri): typedef each struct vairant
                                    for(TypeInfo *struct_variant = type_info->first_struct_variant;
                                        struct_variant != 0;
                                        struct_variant = struct_variant->next)
                                    {
                                        PushTypedef_Struct(sg_arena, struct_variant, &stream);
                                    }
                                } break;
                                case TypeInfoKind_Struct:
                                {
                                    PushTypedef_Struct(sg_arena, type_info, &stream);
                                } break;
                            }
                        }
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                        
                        for(MD_EachNode(type_node, node->first_child))
                        {
                            TypeInfo *type_info = GetTypeInfoFromMap_ByStr(&types_map, type_node->string);
                            if (type_info->kind == TypeInfoKind_Enum)
                            {
                                PushTypeDeclaration_Enum(sg_arena, type_info, &stream);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                            }
                            else if (type_info->kind == TypeInfoKind_Union)
                            {
                                PushTypeDeclaration_Enum(sg_arena, type_info->union_kind_enum_type, &stream);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                                for(TypeInfo *struct_variant = type_info->first_struct_variant;
                                    struct_variant != 0;
                                    struct_variant = struct_variant->next)
                                {
                                    PushTypeDeclaration_Struct(sg_arena, struct_variant, &stream);
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                                }
                                
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("struct "));
                                MD_S8ListPush(sg_arena, &stream, type_info->name);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n{\n\t"));
                                
                                FieldInfo *field_info = type_info->first_field;
                                PushTypedefNameIfBaseElseNormalName(sg_arena, field_info->type, &stream);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit(" "));
                                MD_S8ListPush(sg_arena, &stream, field_info->name);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit(";\n"));
                                
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\tunion\n\t{\n"));
                                field_info = field_info->next;
                                for (;
                                     field_info != 0;
                                     field_info = field_info->next)
                                {
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\t\t"));
                                    PushTypedefNameIfBaseElseNormalName(sg_arena, field_info->type, &stream);
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit(" "));
                                    MD_S8ListPush(sg_arena, &stream, field_info->name);
                                    if (field_info->is_array)
                                    {
                                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("["));
                                        MD_S8ListPush(sg_arena, &stream, field_info->array_len);
                                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("]"));
                                    }
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit(";\n"));
                                }
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\t};\n"));
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("};\n"));
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                            }
                            else if (type_info->kind == TypeInfoKind_Struct)
                            {
                                PushTypeDeclaration_Struct(sg_arena, type_info, &stream);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                            }
                        }
                        
                    }
                    
                    // NOTE(fakhri): serialization functions declaratino
                    {
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("// Serialization Functions Declarations\n"));
                        for(MD_EachNode(type_node, node->first_child))
                        {
                            TypeInfo *type_info = GetTypeInfoFromMap_ByStr(&types_map, type_node->string);
                            if(type_info->kind == TypeInfoKind_Union)
                            {
                                PushFunctionDeclaration(sg_arena, type_info->union_kind_enum_type, &stream, type_info->union_kind_enum_type->serialize_function);
                                
                                for(TypeInfo *struct_variant = type_info->first_struct_variant;
                                    struct_variant != 0;
                                    struct_variant = struct_variant->next)
                                {
                                    PushFunctionDeclaration(sg_arena, struct_variant, &stream, struct_variant->serialize_function);
                                }
                            }
                            
                            PushFunctionDeclaration(sg_arena, type_info, &stream, type_info->serialize_function);
                        }
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    }
                    
                    
                    // NOTE(fakhri): deserialization functions declaratino
                    {
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("// Deserialization Functions Declarations\n"));
                        for(MD_EachNode(type_node, node->first_child))
                        {
                            TypeInfo *type_info = GetTypeInfoFromMap_ByStr(&types_map, type_node->string);
                            if(type_info->kind == TypeInfoKind_Union)
                            {
                                PushFunctionDeclaration(sg_arena, type_info->union_kind_enum_type, &stream, type_info->union_kind_enum_type->deserialize_function);
                                
                                for(TypeInfo *struct_variant = type_info->first_struct_variant;
                                    struct_variant != 0;
                                    struct_variant = struct_variant->next)
                                {
                                    PushFunctionDeclaration(sg_arena, struct_variant, &stream, struct_variant->deserialize_function);
                                }
                            }
                            
                            PushFunctionDeclaration(sg_arena, type_info, &stream, type_info->deserialize_function);
                        }
                        
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    }
                    
                    // NOTE(fakhri): close include guards
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("#endif //"));
                    MD_S8ListPush(sg_arena, &stream, guard_name);
                    
                    MD_String8 h_file_content = MD_S8ListJoin(sg_arena, stream, 0);
                    fprintf(h, "%s", h_file_content.str);
                    printf("generated file %s\n", h_filename.str);
                }
                
                // NOTE(fakhri): generating .c
                {
                    MD_String8List stream = {0};
                    // NOTE(fakhri): serialization functions implementation
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("//\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("// Serialization Functions Definition\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("//\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    for(MD_EachNode(type_node, node->first_child))
                    {
                        TypeInfo *type_info = GetTypeInfoFromMap_ByStr(&types_map, type_node->string);
                        PushFunctionSignatureDefinition(sg_arena, type_info, &stream, 1);
                        
                        switch(type_info->kind)
                        {
                            case TypeInfoKind_Base:
                            {
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\tSG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof("));
                                MD_S8ListPush(sg_arena, &stream, type_info->typedef_alias);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("));\n"));
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\tSG_WRITE_TO_BUFFER(buffer, s, sizeof("));
                                MD_S8ListPush(sg_arena, &stream, type_info->typedef_alias);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("));\n"));
                            } break;
                            case TypeInfoKind_Enum:
                            {
                                PushFunctionDefinition_Enum(sg_arena, type_info, &stream, 1);
                            } break;
                            case TypeInfoKind_Struct:
                            {
                                PushFunctionDefinition_Struct(sg_arena, type_info, &stream, 1);
                            } break;
                            case TypeInfoKind_Union:
                            {
                                FieldInfo *field_info = type_info->first_field;
                                
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\t"));
                                PushFunctionSerializationCall(sg_arena, field_info, &stream);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                                
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\tswitch("));
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("s->"));
                                MD_S8ListPush(sg_arena, &stream, field_info->name);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit(")\n\t{\n"));
                                
                                FieldInfo *enum_variants_field = field_info->type->first_field;
                                field_info = field_info->next;
                                for(;
                                    field_info != 0;
                                    field_info = field_info->next, enum_variants_field = enum_variants_field->next)
                                {
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\t\t"));
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("case "));
                                    MD_S8ListPush(sg_arena, &stream, enum_variants_field->name);
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit(": "));
                                    PushFunctionSerializationCall(sg_arena, field_info, &stream);
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit(" break;\n"));
                                }
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\t}\n"));
                            } break;
                        }
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("}\n"));
                        
                        if (type_info->kind == TypeInfoKind_Union)
                        {
                            TypeInfo *kind_enum_type = type_info->union_kind_enum_type;
                            MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                            PushFunctionSignatureDefinition(sg_arena, kind_enum_type, &stream, 1);
                            PushFunctionDefinition_Enum(sg_arena, kind_enum_type, &stream, 1);
                            MD_S8ListPush(sg_arena, &stream, MD_S8Lit("}\n"));
                            
                            for (TypeInfo *struct_variant = type_info->first_struct_variant;
                                 struct_variant != 0;
                                 struct_variant = struct_variant->next)
                            {
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                                PushFunctionSignatureDefinition(sg_arena, struct_variant, &stream, 1);
                                PushFunctionDefinition_Struct(sg_arena, struct_variant, &stream, 1);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("}\n"));
                            }
                        }
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    }
                    
                    // NOTE(fakhri): deserialization functions implementation
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("//\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("// Deserialization Functions Definition\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("//\n"));
                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    for(MD_EachNode(type_node, node->first_child))
                    {
                        TypeInfo *type_info = GetTypeInfoFromMap_ByStr(&types_map, type_node->string);
                        PushFunctionSignatureDefinition(sg_arena, type_info, &stream, 0);
                        
                        switch(type_info->kind)
                        {
                            case TypeInfoKind_Base:
                            {
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\tSG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof("));
                                MD_S8ListPush(sg_arena, &stream, type_info->typedef_alias);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("));\n"));
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\tSG_READ_FROM_BUFFER(buffer, s, sizeof("));
                                MD_S8ListPush(sg_arena, &stream, type_info->typedef_alias);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("));\n"));
                            } break;
                            case TypeInfoKind_Enum:
                            {
                                PushFunctionDefinition_Enum(sg_arena, type_info, &stream, 0);
                            } break;
                            case TypeInfoKind_Struct:
                            {
                                PushFunctionDefinition_Struct(sg_arena, type_info, &stream, 0);
                            } break;
                            case TypeInfoKind_Union:
                            {
                                FieldInfo *field_info = type_info->first_field;
                                
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\t"));
                                PushFunctionDeserializationCall(sg_arena, field_info, &stream);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                                
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\tswitch("));
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("s->"));
                                MD_S8ListPush(sg_arena, &stream, field_info->name);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit(")\n\t{\n"));
                                
                                FieldInfo *enum_variants_field = field_info->type->first_field;
                                field_info = field_info->next;
                                for(;
                                    field_info != 0;
                                    field_info = field_info->next, enum_variants_field = enum_variants_field->next)
                                {
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\t\t"));
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit("case "));
                                    MD_S8ListPush(sg_arena, &stream, enum_variants_field->name);
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit(": "));
                                    PushFunctionDeserializationCall(sg_arena, field_info, &stream);
                                    MD_S8ListPush(sg_arena, &stream, MD_S8Lit(" break;\n"));
                                }
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\t}\n"));
                            } break;
                        }
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("}\n"));
                        
                        if (type_info->kind == TypeInfoKind_Union)
                        {
                            TypeInfo *kind_enum_type = type_info->union_kind_enum_type;
                            MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                            PushFunctionSignatureDefinition(sg_arena, kind_enum_type, &stream, 0);
                            PushFunctionDefinition_Enum(sg_arena, kind_enum_type, &stream, 0);
                            MD_S8ListPush(sg_arena, &stream, MD_S8Lit("}\n"));
                            
                            for (TypeInfo *struct_variant = type_info->first_struct_variant;
                                 struct_variant != 0;
                                 struct_variant = struct_variant->next)
                            {
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                                PushFunctionSignatureDefinition(sg_arena, struct_variant, &stream, 0);
                                PushFunctionDefinition_Struct(sg_arena, struct_variant, &stream, 0);
                                MD_S8ListPush(sg_arena, &stream, MD_S8Lit("}\n"));
                            }
                        }
                        MD_S8ListPush(sg_arena, &stream, MD_S8Lit("\n"));
                    }
                    MD_String8 c_file_content = MD_S8ListJoin(sg_arena, stream, 0);
                    fprintf(c, "%s", c_file_content.str);
                    printf("generated file %s\n", c_filename.str);
                }
            }
        }
        else
        {
            printf("Couldn't open files, make sure you have a */generated/ folder at the location of the md file\n");
        }
    }
    
    return 0;
}



/*
Copyright (c) 2022 Mouad Fakhri

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/