#include "DeltaDetectBatchQBox.h"
#include "timer.h"
#include <math.h>

BOREALIS_NAMESPACE_BEGIN


void DeltaDetectBatchQBox::setup_impl() throw (AuroraException)
{
  int dim_size;
  SchemaArray *the_array_info;

  vector<SchemaArray::DimensionInformation> theDimInfo;
    if ( get_num_inputs() != 1 )
    {   Throw(aurora_typing_exception,
              "DeltaDetect requires exactly one input stream (not " +
              to_string(get_num_inputs()) + ")");
    }


    set_out_description( 0, get_in_stream( 0 ));
 


    DEBUG << "DeltaDetectBatchQBox input schema " << get_in_schema(0)->as_string();
    DEBUG << "DeltaDetectBatchQBox output schema " << get_out_schema(0)->as_string();

    typed_param("delta_threshold", _delta_threshold);
    DEBUG << "Threshold: " << _delta_threshold;
    TupleDescription in_td(get_in_schema(0));
    int the_field_index = in_td.index_of_field(string("frame"));
    const SchemaField *the_array = in_td.get_schema_field(the_field_index);
    the_array_info  = the_array->get_array();
    _array_size = the_array_info->array_size(); 
    
    _cached_array = DynamicArray::allocate_array(DynamicArray::header_size()+_array_size);
    _cached_array->set_count_size();
    theDimInfo = the_array_info->get_all_dimensions();    
    _frame_offset = in_td.get_offset(the_field_index);


    for(unsigned int x = 0; x < theDimInfo.size(); ++x)
      {
    dim_size = (uint32)(theDimInfo[x]._upper - theDimInfo[x]._lower) + 1;
    _array_dimensions.push_back(dim_size);
      }
    
    int cid;
    _num_chunks = 1;
    
    for(int x = 0; x < 3; ++x)
    {
        ostringstream the_param;
        
        the_param << "chunks_in_dimension." << x;
        typed_param(the_param.str().c_str(),cid);
        _chunks_in_each_dimension.push_back(cid);
        _num_chunks *= cid;
    }
    
    if(_chunks_in_each_dimension.size() != _array_dimensions.size())
      NOTICE << "Require the same number of dimensions and chunks specified in each dimension.";
    _chunk_size = _array_dimensions[0] / _chunks_in_each_dimension[0];

    for(unsigned int x = 1; x < _array_dimensions.size(); ++x)
      _chunk_size *= _array_dimensions[x] / _chunks_in_each_dimension[x];

    _first_flag = 1;
}

DeltaDetectBatchQBox::~DeltaDetectBatchQBox()
{
  _cached_array->modify_array_reference(-1);
}

void DeltaDetectBatchQBox::init_impl() throw (AuroraException)
{
  TupleDescription out_td(get_out_schema(0));
  _output_tuple_size = out_td.get_size();

}

void DeltaDetectBatchQBox::run_impl(QBoxInvocation& inv) throw (AuroraException)
{
    uint32 *element;
    DeqIterator my_deq_iterator = deq(0);
    DynamicArray *new_chunk;
    TupleDescription in_td(get_in_schema(0));
    TupleDescription out_td(get_out_schema(0));
    timer theTime;
    

    // Do stuff...
    while (inv.continue_dequeue_on(my_deq_iterator, 0))
    {
      
      theTime.start();
        const void *data = my_deq_iterator.tuple();
    _min_x = _min_y = 100000;
    _max_x = _max_y = 0;
    _current_array = (DynamicArray *) (*(uint32 *) ((((char *)data) + _frame_offset)));  
    new_chunk = 0;
    

    for(unsigned int x = 0; x < _num_chunks; ++x)
      {
        float theP = calculatePercentChanged(x);
        if(theP >= _delta_threshold)
          {
               map_chunk(x);
               fix_bounds(x);
          }
      }

       if(_min_x !=  100000)
       {
     new_chunk = create_new_chunk();        
         memcpy(enq(0).tuple(), data, _output_tuple_size);
    element = (uint32 *)((uint8 *)enq(0).tuple() + _frame_offset);
    *element  = (uint32) new_chunk;
            
        ++(enq(0));
        get_output(0).notify_enq();
         
       }
     ++my_deq_iterator;

        _first_flag = 0;
    theTime.stop();
    theTime.check("Deltadetect took ");
    theTime.restart();
    }
 }

void DeltaDetectBatchQBox::set_pending_box_state(ptr<AbstractBoxState> packed_box)
{
    //DeltaDetectBoxState* actual_packed_box = static_cast<DeltaDetectBoxState*>(packed_box.get());
    //_pending_box_state.set(new DeltaDetectBoxState(*actual_packed_box));
    _pending_box_state = packed_box.dynamic_cast_to<DeltaDetectBatchBoxState>();
}

ptr<AbstractBoxState> DeltaDetectBatchQBox::pack_state(bool delta_only)
{
  INFO << "Packing state for DeltaDetectBatchQBox named [" << get_name() << "]";
  ptr<DeltaDetectBatchBoxState> filter_box_state(new DeltaDetectBatchBoxState());
  return filter_box_state;
}


void DeltaDetectBatchQBox::addCoord(uint32 dim, uint32 n, uint32 &g)
{

  int theCoordinate;

  if(n == 0 && dim == 0)
    {
      theCoords.insert(theCoords.begin(), 0);
      return;
    }

  if(n == 0)
    {
      theCoords.insert(theCoords.begin(), 0);
      addCoord(dim-1, n, g);
      return;
    }

  if(dim == 0)
    {

      theCoordinate = n;
      theCoords.insert(theCoords.begin(), theCoordinate);
      return;
    }
  else
    {
      g = (int) floor(g / _chunks_in_each_dimension[dim]);
      theCoordinate = (int) floor(n / g);
      n = n - theCoordinate*g;
      theCoords.insert(theCoords.begin(),theCoordinate);
      addCoord(dim-1, n, g);
    }
}


float DeltaDetectBatchQBox::calculatePercentChanged(uint32 chunk_number)
{
 uint32 start_x, start_y, end_x, end_y, offset, row_jump, cutoff_idx;
 uint32 chunk_x, chunk_y, chunk_z;
  float set_length;
  float percent_changed = 0.0;
  uint8 *cache_pos, *in_pos;
  uint8 old_val, new_val;
  uint32 row_count, row_pos;


  uint8 *old_array_start_val = (((uint8 *)_cached_array) + DynamicArray::header_size());
  uint8 *new_array_start_val = (((uint8 *)_current_array) + DynamicArray::header_size());

  uint32 g = _num_chunks;
  theCoords.clear();
  addCoord(2, chunk_number, g);



  // 100% difference for first one
  if(_first_flag)
    return 1;

   set_length = _chunk_size;
   chunk_x = _array_dimensions[0]/_chunks_in_each_dimension[0];
   chunk_y = _array_dimensions[1]/_chunks_in_each_dimension[1];
   chunk_z = _array_dimensions[2]/_chunks_in_each_dimension[2];

  start_x = chunk_x*theCoords[0];
  start_y = chunk_y*theCoords[1];
  end_x = start_x + chunk_x;
  end_y = start_y + chunk_y;

  offset = (_array_dimensions[0]*start_y + start_x)*_array_dimensions[2];
  cache_pos = old_array_start_val + offset;
  in_pos = new_array_start_val + offset;
  row_jump = (_array_dimensions[0] - chunk_x)*_array_dimensions[2];
  row_count = row_pos = 0;
  cutoff_idx = chunk_x*chunk_z;
 
  while(row_count != chunk_y)
  {
        old_val = *cache_pos;
        new_val = *in_pos;
        percent_changed += fabs(old_val - new_val);
        ++cache_pos;
        ++in_pos;
        ++row_pos;
    
    if(row_pos == cutoff_idx)
        {
                // on to the next row
                row_pos = 0;
                ++row_count;
                cache_pos = cache_pos + row_jump;
                in_pos = in_pos + row_jump;
        }

  }

  percent_changed /= set_length;
  percent_changed /= 256.0;
  DEBUG << "Percent changed for chunk " << chunk_number << ": " << percent_changed;
  return percent_changed;
}

void DeltaDetectBatchQBox::map_chunk(int chunk_number)
{
  DEBUG << "Mapping chunk " << chunk_number;
  char *cache_start_val = (char *)(((uint8 *) _cached_array) + DynamicArray::header_size()); 
  char *new_array_start_val = (char *)(((uint8 *) _current_array) + DynamicArray::header_size()); 

  uint32 start_x, start_y, start_z;
  uint32 chunk_x, chunk_y, chunk_z;
  char *src, *dst;
  int row_size, array_row_size, offset;

  chunk_x = _array_dimensions[0]/_chunks_in_each_dimension[0];
  chunk_y = _array_dimensions[1]/_chunks_in_each_dimension[1];
  chunk_z = _array_dimensions[2]/_chunks_in_each_dimension[2];

  start_x = chunk_x*theCoords[0];
  start_y = chunk_y*theCoords[1];
  start_z = chunk_z*theCoords[2];
    
  row_size = chunk_x*chunk_z;
  // calculate initial offset
  
  offset = (_array_dimensions[0]*start_y+start_x)*_array_dimensions[2];
  
  array_row_size = _array_dimensions[0]*_array_dimensions[2]; 
  src = new_array_start_val + offset;
  dst = cache_start_val + offset;
  for(unsigned int y = 0; y < chunk_y; ++y)
    {
        memcpy(dst, src, row_size);
        src = src + array_row_size; 
        dst = dst + array_row_size;
    }
}

void DeltaDetectBatchQBox::fix_bounds(uint chunkNum)
{
  uint32 start_x, start_y, last_x, last_y;
  uint32 chunk_x, chunk_y;
  
  chunk_x = _array_dimensions[0]/_chunks_in_each_dimension[0];
  chunk_y = _array_dimensions[1]/_chunks_in_each_dimension[1];
  

  start_x = chunk_x*theCoords[0];
  start_y = chunk_y*theCoords[1];
  last_x = start_x + chunk_x;
  last_y = start_y + chunk_y;

  if(start_x < _min_x)
    _min_x = start_x;
  if(start_y < _min_y)
    _min_y = start_y;
  if(last_x > _max_x)
    _max_x = last_x;
  if(last_y > _max_y)
    _max_y = last_y;
  

}
uint8 DeltaDetectBatchQBox::getValue( unsigned int x, unsigned int y, unsigned int z, char *array)
{
  int offset = (_array_dimensions[0]*y+x)*_array_dimensions[2]+z;
  return (uint8) *(array+offset);

}

void DeltaDetectBatchQBox::setValue( unsigned int x, unsigned int y, unsigned int z, char *array, uint8 theValue)
{
  int offset = (_array_dimensions[0]*y+x)*_array_dimensions[2]+z;
  *(array+offset) = theValue;

}


DynamicArray *DeltaDetectBatchQBox::create_new_chunk()
{
  DynamicArray *the_chunk;
  char *the_chunk_vals;
  // using current array gives it slightly better vals
  char *the_array_vals = (char *) (((uint8 *) _current_array) + _current_array->header_size());

  uint32 chunk_x, chunk_y, chunk_z;
  uint32 x_dim, y_dim;
  char *src, *dst;
  uint32 chunk_row_jump, array_row_jump, initial_offset;


  x_dim = _max_x - _min_x;
  y_dim = _max_y - _min_y;


  the_chunk = DynamicArray::allocate_array(DynamicArray::header_size() + x_dim*y_dim*3*2);
  the_chunk_vals = (char *) (((uint8 *) the_chunk) + the_chunk->header_size());
 
  //Maintain reference counters
  the_chunk->set_count_size(1,1);
  the_chunk->_start_coord[0] = _min_x;
  the_chunk->_start_coord[1] = _min_y;
  the_chunk->_dim_length[0] = _max_x - _min_x;
  the_chunk->_dim_length[1] = _max_y - _min_y;
  
  the_chunk->_frame_bit = 1;
  
  
  
  chunk_x = x_dim;
  chunk_y = y_dim;
  chunk_z = 3;
  
  initial_offset = (_array_dimensions[0]*_min_y+_min_x)*_array_dimensions[2];
  src = the_array_vals + initial_offset;
  dst = the_chunk_vals;
  chunk_row_jump = x_dim*chunk_z;
  array_row_jump = _array_dimensions[0]*_array_dimensions[2]; 

  for(unsigned int y = 0; y < y_dim; ++y)
  {
    memcpy(dst, src, chunk_row_jump);
    src = src + array_row_jump;
    dst = dst + chunk_row_jump;
  }

    return the_chunk;
}

BOREALIS_NAMESPACE_END


