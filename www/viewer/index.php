<?php

if (!extension_loaded('v8js'))
    die("V8Js missing");

// ********************************************************************

// Add the path to your getevent executable. It is part of FACT++
// and usually resides either in its main or in its build directory.
$getevent = "/home/fact/FACT++/getevent";

// Three paths to calibrated raw data ('cal'), the raw data
// itself ('raw') and your Monte Carlo data ('mc'). Calibrated and
// Monte Carlo data neeeds special preparation.
$path = array(
              "cal" => "/newdata/www/cal/",
              "raw" => "/newdata/raw/",
              "mc"  => "/newdata/www/mc/",
              );

// ********************************************************************

if (isset($_POST['editor1']) || isset($_POST['editor2']))
{
    $isOne = isset($_POST['editor1']);

    $source = $isOne ? $_POST['editor1'] : $_POST['editor2'];
    if (!isset($_POST['files']))
        $name = $isOne ? "proc.js" : "main.js";
    else
        $name = $_POST['files'][0];

    header($_SERVER["SERVER_PROTOCOL"] . " 200 OK");
    header('Cache-Control: public'); // needed for i.e.
    header('Content-Type: text/plain');
    header('Content-Transfer-Encoding: Text');
    header('Content-Disposition: attachment; filename="'.$name.'"');
    print(str_replace("\r", "", $source));
    return;
}

// This is a pretty weird hack because it does first convert
// all data to ascii (json) to print it...
if (isset($_POST['data']))
{
    header($_SERVER["SERVER_PROTOCOL"] . " 200 OK");
    header('Cache-Control: public'); // needed for i.e.
    header('Content-Type: text/plain');
    header('Content-Transfer-Encoding: Text');
    header('Content-Disposition: attachment; filename="'.$_POST['name'].'"');

    $json = json_decode($_POST['data']);

    $n   = count($json[0]);
    $cnt = count($json);

    for ($i=0; $i<$n; $i++)
    {
        print($i." ".$json[0][$i]);

        if ($cnt>1)
            print(" ".$json[1][$i]);
        if ($cnt>2)
            print(" ".$json[2][$i]);
        if ($cnt>3)
            print(" ".$json[3][$i]);
        print("\n");
    }
    return;
}

if (!isset($_POST['file']) || !isset($_POST['event']) || !isset($_POST['pixel']))
{
   /*
    function getList($path)
    {
        $hasdir = false;

        $list = array();
        foreach (new DirectoryIterator($path) as $file)
        {
           if ($file->isDot())
              continue;

           $name = $file->getFilename();


           if ($file->isDir())
           {
               $list[$name] = getList($path."/".$name);
               $hasdir = true;
           }

           if ($file->isFile() && $file->isReadable())
           {
               if (substr($name, 12)!=".fits.fz")
                   continue;

               array_push($list, substr($name, 9, 3));
           }
        }

        if (!$hasdir)
            sort($list);
        return $list;
    }
    */

    function getList(&$list, $path, $ext, $id, $sub = "")
    {
        $hasdir = false;

        $dir = new DirectoryIterator($path."/".$sub);
        foreach ($dir as $file)
        {
           if ($file->isDot())
              continue;

           $name = $file->getFilename();

           if ($file->isDir())
               getList($list, $path, $ext, $id, $sub."/".$name);

           if (!$file->isFile() || !$file->isReadable())
               continue;

           if (substr($name, -strlen($ext))!=$ext)
               continue;

           $rc = substr($name, 0, 4)."/".substr($name, 4, 2)."/".substr($name, 6, 2)."-".substr($name, 9, 3);

           if (!isset($list[$rc]))
               $list[$rc] = 0;

           $list[$rc] |= $id;
        }
    }

    try
    {
        $list = array();
        getList($list, $path['raw'], ".fits.fz",  1);
        getList($list, $path['raw'], ".drs.fits", 2);
        getList($list, $path['cal'], ".fits.fz",  4);
        getList($list, $path['mc'],  ".fits.fz",  8);
        ksort($list);
    }
    catch (Exception $e)
    {
        return header('HTTP/1.0 400 '.$e->getMessage());
    }

    print(json_encode($list));
    return;
}

$event = intval($_POST['event']);
$pixel = intval($_POST['pixel']);

function get($handle, $format, $count = 1)
{
    $size = 0;
    switch ($format)
    {
    case 'd': $size = 8; break;
    case 'L': $size = 4; break;
    case 'S': $size = 2; break;
    case 's': $size = 2; break;
    }

    if ($size==0)
        return;

    $binary = fread($handle, $size*$count);
    $data   = unpack($format.$count, $binary);

    return $count==1 ? $data[1] : $data;
}

//ini_set("memory_limit", "64M");

//define('E_FATAL',  E_ERROR | E_USER_ERROR | E_PARSE | E_CORE_ERROR |
//        E_COMPILE_ERROR | E_RECOVERABLE_ERROR);

$rc = array();
$rc['startPhp'] = microtime(true);

// ============================ Read data from file ==========================
$file = $_POST['file'];

$y = substr($file,  0, 4);
$m = substr($file,  5, 2);
$d = substr($file,  8, 2);
$r = substr($file, 11, 3);

$rootpath  = isset($_POST['calibrated']) && !isset($_POST['drsfile']) ? $path['cal'] : $path['raw'];
$extension = isset($_POST['drsfile'])    ? ".drs.fits" : ".fits.fz";
$filename  = $rootpath.$y."/".$m."/".$d."/".$y.$m.$d."_".$r.$extension;

if (isset($_POST['montecarlo']))
{
    $rootpath  = $path['mc'];
    $extension = ".fits.fz";
    $filename  = $rootpath.$y.$m.$d."_".$r.$extension;
}

if (!file_exists($filename))
    return header("HTTP/1.0 400 File '".$file."' not found.");

$command = $getevent." ".$filename." ".$event." 2> /dev/null";
$file = popen($command, "r");
if (!$file)
    return header('HTTP/1.0 400 Could not open pipe.');

$evt = array();
$fil = array();
$fil['isMC']         = isset($_POST['montecarlo']);
$fil['isDRS']        = isset($_POST['drsfile']);
$fil['isCalibrated'] = isset($_POST['calibrated']);

$fil['runType'] = trim(fread($file, 80));
if (feof($file))
   return header('HTTP/1.0 400 Data not available.');

$fil['runStart']      = get($file, "d");
$fil['runEnd']        = get($file, "d");
$fil['drsFile']       = get($file, "s");
$fil['numEvents']     = get($file, "L");
$fil['scale']         = get($file, "s");
$evt['numRoi']        = get($file, "L");
$evt['numPix']        = get($file, "L");
$evt['eventNumber']   = get($file, "L");
//$evt['triggerNumber'] = get($file, "L");
$evt['triggerType']   = get($file, "S");
$evt['unixTime']      = get($file, "L", 2);

if (isset($_POST['source1']))
{
    // Read the data and copy it from an associative array to an Array
    // (this is just nicer and seems more logical)
    $binary = array();
    for ($i=0; $i<$evt['numPix']; $i++)
        $binary[$i] = fread($file, 2*$evt['numRoi']);
    /*
    $data = array();
    for ($i=0; $i<$evt['numPix']; $i++)
    {
        $var = get($file, "s", $evt['numRoi']);
        $data[$i] = array();
        for ($j=0; $j<$evt['numRoi']; $j++)
            $data[$i][$j] = $var[$j+1]*0.48828125; // dac -> mV
    }*/
}

if (feof($file))
   return header('HTTP/1.0 400 Data from file incomplete.');

pclose($file);

if ($fil['numEvents']==0)
    return header('HTTP/1.0 400 Could not read event.');

// =========================== Decode trigger type ===========================

$typ = $evt['triggerType'];
$evt['trigger'] = array();
if ($typ!=0 && ($typ & 0x8703)==0)
    array_push($evt['trigger'], "PHYS");
if (($typ&0x0100)!=0)
    array_push($evt['trigger'], "LPext");
if (($typ&0x0200)!=0)
    array_push($evt['trigger'], "LPint");
if (($typ&0x0400)!=0)
    array_push($evt['trigger'], "PED");
if (($typ&0x8000)!=0)
    array_push($evt['trigger'], "TIM");
if (($typ&0x0001)!=0)
    array_push($evt['trigger'], "EXT1");
if (($typ&0x0002)!=0)
    array_push($evt['trigger'], "EXT2");

// =============================== Some data =================================

//require_once("neighbors.php");

$rc['command'] = "getevent ".$filename." ".$event;
//$rc['neighbors'] = $neighbors;
$rc['event'] = $evt;
$rc['file']  = $fil;
$rc['event']['index'] = $event;
$rc['event']['pixel'] = $pixel;
//$rc['waveform'] = $data[$pixel];

// Get execution times
$now = microtime(true);

if (isset($_POST['source1']))
{
    // =============================== Run Javascript ============================

/*
$JS = <<< EOT
require("path/to/module1");
EOT;

$v8 = new V8Js();
$v8->setModuleLoader(function($module) {
  switch ($module) {
    case 'path/to/module1':
      return 'print(' . json_encode($module . PHP_EOL) . ');require("./module2");';

    case 'path/to/module2':
      return 'print(' . json_encode($module . PHP_EOL) . ');require("../../module3");';

    default:
      return 'print(' . json_encode($module . PHP_EOL) . ');';
  }
});
*/

    // V8Js global methods:  exit, print, sleep, var_dump, require
    //$v8 = new V8Js("$", array("data"=>"data"), extensions, flags, millisecond, bytes);
    $v8 = new V8Js("$"/*, array("data"=>"data")*/);

    //$v8 = new V8Js("$", array("data"=>"data"));

    //V8Js::registerExtension("exit", "1");
    //$v8 = new V8Js("$", array(), array("exit"));

    //$v8->func = function($a) { echo "Closure with param $a\n"; };
    //$v8->greeting = "From PHP with love!";

    // This is much faster than the variables option in the constructor

    $roi = $evt['numRoi'];

    //$v8->data   = $data;
    //$v8->test   = array();;
    //$v8->data   = array();
    $v8->nroi      = $evt['numRoi'];
    $v8->npix      = $evt['numPix'];
    $v8->trigger   = $evt['trigger'];
    $v8->event     = $evt;
    $v8->file      = $fil;
//    $v8->neighbors = $neighbors;
    $v8->clone     = function($data) { return $data; };
    $v8->scale     = $fil['scale']==0 ? 2000./4096. : 1./$fil['scale'];
    $v8->unpack    = function($i)
    {
        global $binary, $roi, $v8;
        $u = unpack("s".$roi, $binary[$i]);
        $arr = array();
        for ($i=0; $i<$roi; $i++)
            $arr[$i] = $u[$i+1]*$v8->scale;
        return $arr;
    };

    //  10, 445, 91.75 MiB
    // 376, 720, 91.75 MiB

    $internal = file_get_contents("internal.js");

    // Buffer output from javascript
    ob_start();

    $rc['timeJS']  = array();
    $rc['startJs'] = microtime(true);

    try
    {
        $v8->executeString($internal, 'internal.js');

        // We unpack the data pixel by pixel and copy the array directly to the
        // Javasscript array. This significantly decreases memory usage because
        // we need only one of the super memory hungry php arrays of size ROI
        // instead of 1440.
        $JS = "$.event.time=new Date($.event.unixTime[1]*1000+$.event.unixTime[2]/1000); $.data = new Array($.event.numPix); for (var i=0; i<$.event.numPix; i++) $.data[i] = $.unpack(i);";
        $v8->executeString($JS, 'internal');

        $rc['timeJs'][0] = (microtime(true) - $rc['startJs']);

        $JS = "'use strict'; function proc(pixel){\n".$_POST['source1']."\n};proc(".$pixel.");";
        $rc['waveform'] = $v8->executeString($JS, 'proc');

        $rc['timeJs'][1] = (microtime(true) - $rc['startJs']);

        if (isset($_POST['source2']))
        {
            $JS = "'use strict'; (function main(){\n".$_POST['source2']."\n})();";
            $rc['ret'] = $v8->executeString($JS, 'main');
        }

        // This is supposed to work, but it seems it does not...
        //$v8->proc = $rc['ret']->proc;
        //$rc['ret'] = $v8->executeString('PHP.proc();', 'proc');
    }
    catch (V8JsException $e)
    {
        $rc['err'] = array();
        $rc['err']['message']    = $e->getMessage();
        $rc['err']['file']       = $e->getJsFileName();
        $rc['err']['sourceLine'] = $e->getJsSourceLine();
        $rc['err']['lineNumber'] = $e->getJsLineNumber()-1;
        $rc['err']['trace']      = $e->getJsTrace();
    }

    $rc['timeJs'][2]  = (microtime(true) - $rc['startJs']);

    // Copy output buffer and clean it
    $rc['debug'] = ob_get_contents();
    ob_end_clean();
}

$rc['memory']  = memory_get_peak_usage(true)/1024/1024;
$rc['timePhp'] = (microtime(true) - $rc['startPhp']);

//unset($rc['neighbors']);

// Output result as JSON object
print(json_encode($rc));
?>
