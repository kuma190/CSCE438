import java.io.IOException;
import java.util.StringTokenizer;
import java.lang.String;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;

public class SleepTweets {

  public static class TokenizerMapper
       extends Mapper<Object, Text, Text, IntWritable>{

    private final static IntWritable one = new IntWritable(1);
    private Text word = new Text();

    public void map(Object key, Text value, Context context
                    ) throws IOException, InterruptedException {
        StringTokenizer itr = new StringTokenizer(value.toString(),"\n");
        String hour = "";
        while (itr.hasMoreTokens()) {
          //get hour
            String line = itr.nextToken();
            if (line.length() != 0 && line.charAt(0)=='T'){
                //System.out.println(line);
                StringTokenizer tk = new StringTokenizer(line);
                if (tk.hasMoreTokens()) tk.nextToken();
                if (tk.hasMoreTokens()) tk.nextToken();
                if (tk.hasMoreTokens()){
                    String timestamp = tk.nextToken();
                    hour = timestamp.substring(0,2);
                    //System.out.println(hour);
                }
            }
            
        //word.set(itr.nextToken());
        //context.write(word, one);
        if (itr.hasMoreTokens()) itr.nextToken();
        if (itr.hasMoreTokens()){
            String postLine = itr.nextToken();
                if (postLine.length() != 0 && postLine.charAt(0)=='W'){
                  //decide if post has sleep in it
                    //System.out.println(postLine);
                    StringTokenizer ptk = new StringTokenizer(postLine);
                    if (ptk.hasMoreTokens()) ptk.nextToken();
                    while (ptk.hasMoreTokens()){
                        String post = ptk.nextToken();
                        //System.out.println(post);
                        if (post.indexOf("sleep")!= -1){
                            //System.out.println(hour+" "+postLine);
                            word.set(hour);
                            context.write(word, one);
			    break;
                        }
                    }
                }
            }
        }

    }
  }

  public static class IntSumReducer
       extends Reducer<Text,IntWritable,Text,IntWritable> {
    private IntWritable result = new IntWritable();

    public void reduce(Text key, Iterable<IntWritable> values,
                       Context context
                       ) throws IOException, InterruptedException {
      int sum = 0;
      for (IntWritable val : values) {
        sum += val.get();
      }
      result.set(sum);
      context.write(key, result);
    }
  }

  public static void main(String[] args) throws Exception {
    Configuration conf = new Configuration();
    conf.set("textinputformat.record.delimiter","\n\n");
    Job job = Job.getInstance(conf, "word count");
    //conf.set("textinputformat.record.delimiter", "\r\n\r\n");
    job.setJarByClass(SleepTweets.class);
    job.setMapperClass(TokenizerMapper.class);
    job.setCombinerClass(IntSumReducer.class);
    job.setReducerClass(IntSumReducer.class);
    job.setOutputKeyClass(Text.class);
    job.setOutputValueClass(IntWritable.class);
    FileInputFormat.addInputPath(job, new Path(args[0]));
    FileOutputFormat.setOutputPath(job, new Path(args[1]));
    System.exit(job.waitForCompletion(true) ? 0 : 1);
  }
}


