
#ifndef SIMPLE_CONVERTER_BOX_H
#define SIMPLE_CONVERTER_BOX_H

#include "ConverterBox.h"

BOREALIS_NAMESPACE_BEGIN;

/// Converts delimited or fixed-length records to tuples on a single
/// stream.
class SimpleConverterBox : public ConverterBox
{
  protected:
    /// Create a converter box where each input record is terminated
    /// with a specific character.
    ///
    /// @param schema     the schema generated by the converter box
    /// @param delim      the input delimiter (e.g., '\n')
    SimpleConverterBox(TupleDescription schema, char delim);

    /// Create a converter box where each input is of a fixed size.
    ///
    /// @param schema     the schema generated by the converter box
    /// @param length     the length of each input tuple
    SimpleConverterBox(TupleDescription schema, unsigned int length);

    /// Destructor.
    ~SimpleConverterBox();

    /// Handles data by converting input records and enqueueing
    /// resultant data.  Should not be overridden by subclasses (if
    /// you would need to override this, subclass ConverterBox
    /// directly).
    virtual unsigned int handle_data(constbuf input)
        throw (AuroraException);

    /// Convert an input record.  Must be implemented in the subclass.
    ///
    /// @param data   the beginning of an input record
    /// @param length the length of the input record
    /// @param out    a pointer to a buffer in which to store the converted data
    /// @return true if the record was successfully converted
    virtual bool convert_tuple(const char *data, unsigned int length,
                              char *out)
        throw (AuroraException) = 0;

  private:
    char _delim;
    unsigned int _length;

    // Output buffer (size is tuple length)
    char *_output;

    void check_schema_length(unsigned int tuple_size) throw (aurora_illegal_argument_exception);
};

BOREALIS_NAMESPACE_END;

#endif
