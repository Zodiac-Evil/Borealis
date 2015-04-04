#include "FaceDetectBatchQBox.h"
#include <math.h>
#include "timer.h"

BOREALIS_NAMESPACE_BEGIN

void FaceDetectBatchQBox::setup_impl() throw (AuroraException)
{
    int dim_size;
    SchemaArray *the_array_info;

    vector<SchemaArray::DimensionInformation> theDimInfo;
    
    if ( get_num_inputs() != 1 )
    {   Throw(aurora_typing_exception,
              "FaceDetectBatch requires exactly one input stream (not " +
              to_string(get_num_inputs()) + ")");
    }


    set_out_description( 0, get_in_stream( 0 ));
 


    DEBUG << "FaceDetectBatchQBox input schema " << get_in_schema(0)->as_string();
    DEBUG << "FaceDetectBatchQBox output schema " << get_out_schema(0)->as_string();

    TupleDescription in_td(get_in_schema(0));
    int the_field_index = in_td.index_of_field(string("frame"));
    SchemaField *the_array = const_cast<SchemaField *>(in_td.get_schema_field(the_field_index));
    the_array_info  = the_array->get_array();
    _array_size = the_array_info->array_size(); 
    theDimInfo = the_array_info->get_all_dimensions();
    
    _frame_offset = in_td.get_offset(the_field_index);
    
    for(unsigned int x = 0; x < theDimInfo.size(); ++x)
      {
        dim_size = (uint32)(theDimInfo[x]._upper - theDimInfo[x]._lower) + 1;
        _array_dimensions.push_back(dim_size);
      }
    // the cache holds values and we ship out a copy of it when the frame bit arrives
    // will need to include a bounding box for region of change in schema
    
    _cached_array = DynamicArray::allocate_array(DynamicArray::header_size() +_array_size);
    _cached_array->set_count_size();
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
    /*_chunks_in_each_dimension.push_back(8);
    _chunks_in_each_dimension.push_back(6);
    _chunks_in_each_dimension.push_back(1);
    _num_chunks = 48;
    */
    const char *cascade_name = "/u/jennie/Array_Research/opencv-1.0.0/data/haarcascades/haarcascade_frontalface_alt2.xml";
    _cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
    if(_cascade == 0)
        DEBUG << "Failed to load cascade";
    _storage = cvCreateMemStorage(0);
}


FaceDetectBatchQBox::~FaceDetectBatchQBox()
{
    _cached_array->modify_array_reference(-1);
}

void FaceDetectBatchQBox::init_impl() throw (AuroraException)
{
  TupleDescription out_td(get_out_schema(0));
  _output_tuple_size = out_td.get_size();

}

void FaceDetectBatchQBox::run_impl(QBoxInvocation& inv) throw (AuroraException)
{

  uint32 *element;
  timer theTime;

    DeqIterator my_deq_iterator = deq(0);

    TupleDescription in_td(get_in_schema(0));
    TupleDescription out_td(get_out_schema(0));

    // Do stuff...
    while (inv.continue_dequeue_on(my_deq_iterator, 0))
    {
      theTime.start();
        const void *data = my_deq_iterator.tuple();
        //Tuple input_tuple = Tuple::factory(data);

       // unconditionally pass through
       memcpy(enq(0).tuple(), data, _output_tuple_size);
       DynamicArray *the_chunk = (DynamicArray *) (*(uint32 *) ((((char *)data) + _frame_offset)));
      DynamicArray *the_out_array;
      the_out_array = DynamicArray::allocate_array(DynamicArray::header_size() +_array_size);
      map_chunk(the_chunk);
      
      detect_on_updates();      
      memcpy((uint8 *)the_out_array, (uint8 *)_cached_array, _cached_array->header_size()+_array_size);
        
              
        element = (uint32 *)((uint8 *)enq(0).tuple() + _frame_offset);
        *element  = (uint32) the_out_array;
        ++(enq(0));
        get_output(0).notify_enq();
      ++my_deq_iterator;
        theTime.stop();
    /* TODO: debug this debug code
         for(uint x = _min_x; x < _max_x; ++x)
            {
                set_pixel_red(x, _min_y, the_out_array);
                set_pixel_red(x, _max_y, the_out_array);
            }
            
        for(uint y = _min_y; y < _max_y; ++y)
        {
            set_pixel_red(_min_x, y, the_out_array);
            set_pixel_red(_max_x, y, the_out_array);
        }
    */
    
    NOTICE << "Face detect took: " << theTime.elapsed_time();
    the_chunk->modify_array_reference(-1);
    theTime.restart();
    
    }
    DEBUG << "Exiting FD.";
 }

void FaceDetectBatchQBox::set_pixel_red(uint32 x, uint32 y, DynamicArray *the_out)
{
    uint32 offset = (_array_dimensions[0]*y + x)*_array_dimensions[2];
    uint8 *out_values = (uint8 *) (((uint8 *)the_out) + _cached_array->header_size());
    
    *(out_values + offset) = 255;
    *(out_values + offset+1) = 0;
    *(out_values + offset+2) = 0;
}
void FaceDetectBatchQBox::add_coord(uint32 dim, uint32 n, uint32 &g)
{

  int theCoordinate;

  if(n == 0 && dim == 0)
    {
      _the_coords.insert(_the_coords.begin(), 0);
      return;
    }

  if(n == 0)
    {
      _the_coords.insert(_the_coords.begin(), 0);
      add_coord(dim-1, n, g);
      return;
    }

  if(dim == 0)
    {

      theCoordinate = n;
      _the_coords.insert(_the_coords.begin(), theCoordinate);
      return;
    }
  else
    {
      g = (int) floor(g / _chunks_in_each_dimension[dim]);
      theCoordinate = (int) floor(n / g);
      n = n - theCoordinate*g;
      _the_coords.insert(_the_coords.begin(),theCoordinate);
      add_coord(dim-1, n, g);
    }
}


void FaceDetectBatchQBox::map_chunk(DynamicArray *the_chunk)
{
  char *the_array_vals = (char *)(((uint8 *) _cached_array) + _cached_array->header_size()); 
  char *the_chunk_vals = (char *)(((uint8 *) the_chunk) + the_chunk->header_size()); 
  char *src, *dst;
  uint32 chunk_row_jump, array_row_jump;
  uint32 row_size, array_offset;  
  uint32 chunk_x, chunk_y, chunk_z;

  
  chunk_x = the_chunk->_dim_length[0];
  chunk_y = the_chunk->_dim_length[1];
  chunk_z = 3;

    _min_x = the_chunk->_start_coord[0];
    _max_x = the_chunk->_start_coord[0] + the_chunk->_dim_length[0];
    _min_y = the_chunk->_start_coord[1];
    _max_y = the_chunk->_start_coord[1] + the_chunk->_dim_length[1];;
    
    
  array_offset = (_array_dimensions[0]*_min_y+_min_x)*_array_dimensions[2]; 
  array_row_jump = _array_dimensions[0]*_array_dimensions[2]; 
  chunk_row_jump = chunk_x*chunk_z;
  row_size = chunk_row_jump;
  
  dst = the_array_vals + array_offset;
  src = the_chunk_vals;
  
  for(unsigned int y = 0; y < chunk_y; ++y)
    {    
      memcpy(dst, src, row_size);
      src = src + chunk_row_jump;
      dst = dst + array_row_jump;
    }
    
}

void FaceDetectBatchQBox::detect_on_updates()
{
   uint32 img_width = _max_x - _min_x;
   uint32 img_height = _max_y - _min_y;
   uint8 *cache_vals = (uint8 *)(((uint8 *) _cached_array) + _cached_array->header_size());   
   IplImage *image = cvCreateImage( cvSize(img_width,img_height),  IPL_DEPTH_8U, 3);
   DEBUG << "Bounding box (" << _min_x << ", " << _min_y << ") (" << _max_x << ", " << _max_y << ")";
   DEBUG << "Size: " << img_width << ", " << img_height;
   for(unsigned int x = 0; x < img_width; ++x)
          for(unsigned int y = 0; y < img_height; ++y)
          {
            ((uchar*)(image->imageData + image->widthStep*y))[x*_array_dimensions[2]] =  cache_vals[(_min_y+y)*_array_dimensions[0]*_array_dimensions[2]+(_min_x+x)*_array_dimensions[2]];
            ((uchar*)(image->imageData + image->widthStep*y))[x*_array_dimensions[2]+1] =  cache_vals[(_min_y+y)*_array_dimensions[0]*_array_dimensions[2]+(_min_x+x)*_array_dimensions[2]+1];
            ((uchar*)(image->imageData + image->widthStep*y))[x*_array_dimensions[2]+2] =  cache_vals[(_min_y+y)*_array_dimensions[0]*_array_dimensions[2]+(_min_x+x)*_array_dimensions[2]+2];
          }
    detect_on_region(image);
    for(unsigned int x = 0; x < img_width; ++x)
          for(unsigned int y = 0; y < img_height; ++y)
          {
            cache_vals[(_min_y+y)*_array_dimensions[0]*_array_dimensions[2]+(_min_x+x)*_array_dimensions[2]]   = ((uchar*)(image->imageData + image->widthStep*y))[x*_array_dimensions[2]];
            cache_vals[(_min_y+y)*_array_dimensions[0]*_array_dimensions[2]+(_min_x+x)*_array_dimensions[2]+1] = ((uchar*)(image->imageData + image->widthStep*y))[x*_array_dimensions[2]+1];
            cache_vals[(_min_y+y)*_array_dimensions[0]*_array_dimensions[2]+(_min_x+x)*_array_dimensions[2]+2] = ((uchar*)(image->imageData + image->widthStep*y))[x*_array_dimensions[2]+2];
          }
    cvReleaseImage(&image); 
    
}

void FaceDetectBatchQBox::detect_on_region(IplImage *img)
{
    DEBUG << "Doing detection!";
static CvScalar colors[] = 
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    double scale = 1.3;
    IplImage* gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
    IplImage* small_img = cvCreateImage( cvSize( cvRound (img->width/scale),
                         cvRound (img->height/scale)),
                     8, 1 );
    int i;

    cvCvtColor( img, gray, CV_BGR2GRAY );
    cvResize( gray, small_img, CV_INTER_LINEAR );
    cvEqualizeHist( small_img, small_img );
    cvClearMemStorage( _storage );

    if( _cascade )
    {
        DEBUG << "Have cascade!";
        double t = (double)cvGetTickCount();
        CvSeq* faces = cvHaarDetectObjects( small_img, _cascade, _storage,
                                            1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                            cvSize(30, 30) );
        t = (double)cvGetTickCount() - t;
        DEBUG << "detection time = " << ((double)cvGetTickFrequency()*1000.) ;
        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
            DEBUG << "Drawing detection!";
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
            CvPoint center;
            int radius;
            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);
            cvCircle( img, center, radius, colors[i%8], 3, 8, 0 );
        }
    }

    cvShowImage( "result", img );
    cvReleaseImage( &gray );
    cvReleaseImage( &small_img );   
}



void FaceDetectBatchQBox::set_pending_box_state(ptr<AbstractBoxState> packed_box)
{
    //FaceDetectBatchBoxState* actual_packed_box = static_cast<FaceDetectBatchBoxState*>(packed_box.get());
    //_pending_box_state.set(new FaceDetectBatchBoxState(*actual_packed_box));
    _pending_box_state = packed_box.dynamic_cast_to<FaceDetectBatchBoxState>();
}

ptr<AbstractBoxState> FaceDetectBatchQBox::pack_state(bool delta_only)
{
  INFO << "Packing state for FaceDetectBatchQBox named [" << get_name() << "]";
  ptr<FaceDetectBatchBoxState> filter_box_state(new FaceDetectBatchBoxState());
  return filter_box_state;
}


BOREALIS_NAMESPACE_END
