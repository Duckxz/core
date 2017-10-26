/*
 *	Serial Library by Parra Studios
 *	Copyright (C) 2016 - 2017 Vicente Eduardo Ferrer Garcia <vic798@gmail.com>
 *
 *	A cross-platform library for managing multiple serialization and deserialization formats.
 *
 */

/* -- Headers -- */

#include <rapid_json_serial/rapid_json_serial_impl.h>

#include <log/log.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

/* -- Private Methods -- */

static void rapid_json_serial_impl_serialize_value(value v, rapidjson::Value * json_v, rapidjson::Document::AllocatorType & allocator);

static char * rapid_json_serial_impl_document_stringify(rapidjson::Document * document, size_t * size);

static value rapid_json_serial_impl_deserialize_value(const rapidjson::Value * v);

/* -- Methods -- */

const char * rapid_json_serial_impl_extension()
{
	static const char extension[] = "json";

	return extension;
}

serial_impl_handle rapid_json_serial_impl_initialize()
{
	rapidjson::Document * document = new rapidjson::Document();

	if (document == nullptr)
	{
		return NULL;
	}

	return (serial_impl_handle)document;
}

void rapid_json_serial_impl_serialize_value(value v, rapidjson::Value * json_v, rapidjson::Document::AllocatorType & allocator)
{
	type_id id = value_type_id(v);

	if (id == TYPE_BOOL)
	{
		boolean b = value_to_bool(v);

		json_v->SetBool(b == 1L ? true : false);
	}
	else if (id == TYPE_CHAR)
	{
		char str[1];

		size_t length = 1;

		str[0] = value_to_char(v);

		json_v->SetString(str, length);
	}
	else if (id == TYPE_SHORT)
	{
		short s = value_to_short(v);

		int i = (int)s;

		json_v->SetInt(i);
	}
	else if (id == TYPE_INT)
	{
		int i = value_to_int(v);

		json_v->SetInt(i);
	}
	else if (id == TYPE_LONG)
	{
		long l = value_to_long(v);

		log_write("metacall", LOG_LEVEL_WARNING, "Casting long to int64_t (posible incompatible types) in RapidJSON implementation");

		json_v->SetInt64(l);
	}
	else if (id == TYPE_FLOAT)
	{
		float f = value_to_float(v);

		json_v->SetFloat(f);
	}
	else if (id == TYPE_DOUBLE)
	{
		double d = value_to_double(v);

		json_v->SetDouble(d);
	}
	else if (id == TYPE_STRING)
	{
		const char * str = value_to_string(v);

		size_t size = value_type_size(v);

		size_t length = size > 0 ? size - 1 : 0;

		json_v->SetString(str, length);
	}
	else if (id == TYPE_BUFFER)
	{
		/* TODO: Implement array-like map */
	}
	else if (id == TYPE_ARRAY)
	{
		rapidjson::Value & json_array = json_v->SetArray();

		value * value_array = value_to_array(v);

		size_t array_size = value_type_size(v) / sizeof(const value);

		for (size_t iterator = 0; iterator < array_size; ++iterator)
		{
			value current_value = value_array[iterator];

			rapidjson::Value json_inner_value;

			rapid_json_serial_impl_serialize_value(current_value, &json_inner_value, allocator);

			json_array.PushBack(json_inner_value, allocator);
		}
	}
	else if (id == TYPE_MAP)
	{
		rapidjson::Value & json_map = json_v->SetObject();

		value * value_map = value_to_map(v);

		size_t map_size = value_type_size(v) / sizeof(const value);

		for (size_t iterator = 0; iterator < map_size; ++iterator)
		{
			value tupla = value_map[iterator];

			value * tupla_array = value_to_array(tupla);

			rapidjson::Value json_member, json_inner_value;

			rapid_json_serial_impl_serialize_value(tupla_array[0], &json_member, allocator);

			rapid_json_serial_impl_serialize_value(tupla_array[1], &json_inner_value, allocator);

			json_map.AddMember(json_member, json_inner_value, allocator);
		}
	}
	else if (id == TYPE_PTR)
	{
		const size_t PTR_STR_MAX_SIZE = 19; /* 16 (64-bit pointer to string) + 2 (0x prefix) + '\0' */

		char ptr_str[PTR_STR_MAX_SIZE] = { 0 };

		size_t length = 0;

		value_stringify(v, ptr_str, PTR_STR_MAX_SIZE, &length);

		json_v->SetString(ptr_str, length);
	}
}

char * rapid_json_serial_impl_document_stringify(rapidjson::Document * document, size_t * size)
{
	char * buffer_str;

	size_t buffer_size, buffer_str_size;

	rapidjson::StringBuffer buffer;

	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

	document->Accept(writer);

	/* StringBuffer does not contain '\0' character so buffer size equals to buffer_str length */
	buffer_size = buffer.GetSize();

	buffer_str_size = buffer_size + 1;

	buffer_str = static_cast<char *>(malloc(sizeof(char) * buffer_str_size));

	if (buffer_str == NULL)
	{
		log_write("metacall", LOG_LEVEL_ERROR, "Invalid string allocation for document stringifycation in RapidJSON implementation");

		return NULL;
	}

	strncpy(buffer_str, buffer.GetString(), buffer_size);

	buffer_str[buffer_size] = '\0';

	*size = buffer_str_size;

	return buffer_str;
}

char * rapid_json_serial_impl_serialize(serial_impl_handle handle, value v, size_t * size)
{
	rapidjson::Document * document = (rapidjson::Document *)handle;

	if (handle == NULL || v == NULL || size == NULL)
	{
		log_write("metacall", LOG_LEVEL_ERROR, "Serialization called with wrong arguments in RapidJSON implementation");

		return NULL;
	}

	rapidjson::Document::AllocatorType & allocator = document->GetAllocator();

	rapid_json_serial_impl_serialize_value(v, document, allocator);

	return rapid_json_serial_impl_document_stringify(document, size);
}

value rapid_json_serial_impl_deserialize_value(const rapidjson::Value * v)
{
	if (v->IsBool() == true)
	{
		return value_create_bool(v->GetBool() == true ? 1L : 0L);
	}
	else if (v->IsString() == true && v->GetStringLength() == 1)
	{
		const char * str = v->GetString();

		return value_create_char(str[0]);
	}
	else if (v->IsInt() == true)
	{
		int i = v->GetInt();

		return value_create_int(i);
	}
	else if (v->IsUint() == true)
	{
		unsigned int ui = v->GetUint();

		log_write("metacall", LOG_LEVEL_WARNING, "Casting unsigned integer to integer (posible overflow) in RapidJSON implementation");

		return value_create_int((int)ui);
	}
	else if (v->IsInt64() == true)
	{
		int64_t i = v->GetInt64();

		return value_create_long((long)i);
	}
	else if (v->IsUint64() == true)
	{
		uint64_t ui = v->GetUint64();

		log_write("metacall", LOG_LEVEL_WARNING, "Casting unsigned long to int (posible overflow) in RapidJSON implementation");

		return value_create_long((long)ui);
	}
	else if (v->IsFloat() == true || v->IsLosslessFloat() == true)
	{
		float f = v->GetFloat();

		return value_create_float(f);
	}
	else if (v->IsDouble() == true || v->IsLosslessDouble() == true)
	{
		double d = v->GetDouble();

		return value_create_double((double)d);
	}
	else if (v->IsString() == true && v->GetStringLength() > 1)
	{
		rapidjson::SizeType length = v->GetStringLength();

		const char * str = v->GetString();

		return value_create_string(str, (size_t)length);
	}
	else if (v->IsArray() == true && v->Empty() == false)
	{
		rapidjson::SizeType size = v->Size();

		value * values = static_cast<value *>(malloc(sizeof(value) * size));

		size_t index = 0;

		if (values == NULL)
		{
			return NULL;
		}

		for (rapidjson::Value::ConstValueIterator it = v->Begin(); it != v->End(); ++it)
		{
			values[index++] = rapid_json_serial_impl_deserialize_value(it);
		}

		return value_create_array(values, size);
	}
	else if (v->IsObject() == true)
	{
		const rapidjson::SizeType size = v->MemberCount();

		value * tuples = static_cast<value *>(malloc(sizeof(value) * size));

		size_t index = 0;

		if (tuples == NULL)
		{
			return NULL;
		}

		for (rapidjson::Value::ConstMemberIterator it = v->MemberBegin(); it != v->MemberEnd(); ++it)
		{
			const value tupla[] =
			{
				rapid_json_serial_impl_deserialize_value(&it->name),
				rapid_json_serial_impl_deserialize_value(&it->value)
			};

			tuples[index++] = value_create_array(tupla, sizeof(tupla) / sizeof(tupla[0]));
		}

		value v = value_create_map(tuples, size);

		free(tuples);

		return v;
	}

	log_write("metacall", LOG_LEVEL_ERROR, "Unsuported value type in RapidJSON implementation");

	return NULL;
}

value rapid_json_serial_impl_deserialize(serial_impl_handle handle, const char * buffer, size_t size)
{
	rapidjson::Document * document = (rapidjson::Document *)handle;

	if (handle == NULL || buffer == NULL || size == 0)
	{
		log_write("metacall", LOG_LEVEL_ERROR, "Deserialization called with wrong arguments in RapidJSON implementation");

		return NULL;
	}

	if (document->Parse(buffer, size - 1).HasParseError() == true)
	{
		log_write("metacall", LOG_LEVEL_ERROR, "Invalid parsing of document (%s) in RapidJSON implementation", buffer);

		delete document;

		return NULL;
	}

	return rapid_json_serial_impl_deserialize_value(document);
}

int rapid_json_serial_impl_destroy(serial_impl_handle handle)
{
	rapidjson::Document * document = (rapidjson::Document *)handle;

	if (document != nullptr)
	{
		delete document;
	}

	return 0;
}