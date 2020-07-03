#ifndef LOOP_ADAPT_PARAMETER_VALUE_TYPES_H
#define LOOP_ADAPT_PARAMETER_VALUE_TYPES_H

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef LA_MIN
#define LA_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef LA_MAX
#define LA_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define LOOP_ADAPT_PARAMETER_TYPE_STR_MAXLENGTH 256


#define LOOP_ADAPT_PARAMETER_VALUE_TYPE_MIN LOOP_ADAPT_PARAMETER_TYPE_BOOL
typedef enum {
    LOOP_ADAPT_PARAMETER_TYPE_BOOL = 0,
    LOOP_ADAPT_PARAMETER_TYPE_INT,
    LOOP_ADAPT_PARAMETER_TYPE_CHAR,
    LOOP_ADAPT_PARAMETER_TYPE_LONG,
    LOOP_ADAPT_PARAMETER_TYPE_ULONG,
    LOOP_ADAPT_PARAMETER_TYPE_UINT,
    LOOP_ADAPT_PARAMETER_TYPE_STR,
    LOOP_ADAPT_PARAMETER_TYPE_DOUBLE,
    LOOP_ADAPT_PARAMETER_TYPE_FLOAT,
    LOOP_ADAPT_PARAMETER_TYPE_PTR,
    LOOP_ADAPT_PARAMETER_TYPE_MAX,
    LOOP_ADAPT_PARAMETER_TYPE_INVALID,
} ParameterValueType_t;

typedef struct _ParameterValue {
    ParameterValueType_t type;
    union {
        unsigned int bval:1;
        int ival;
        char cval;
        double dval;
        float fval;
        char* sval;
        void* pval;
        unsigned int uval;
        long lval;
        unsigned long ulval;
    } value;
} ParameterValue;



#endif /* LOOP_ADAPT_PARAMETER_VALUE_TYPES_H */
