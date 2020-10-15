# WSRTI

### Overview
The WSTRI library aims to facilitate interoperability of web and HPC technologies by helping HPC application developers expose state and data to a web-based client.

The library is split into two components, assuming a client-server application model. A set of server-side utility libraries are provided for exposing state, RPC, and generic data from a C++ application, and client-side web-based library written primarily in JS is provided for recieving and displaying state/data/RPC commands streamed via WebSockets. 

Each of the sub-components have their own readme documenting individual usage. A wiki documenting full library usage for instrumenting a HPC application is forthcoming.

### Motivation

Interactive HPC applications typically follow the client-server application model, where a 'server' runs on one or more compute nodes of a high performance computing system, and a 'client' runs elsewhere, for example locally on a user machine, on the login node of the HPC system, or even on a separate web-server. Most existing HPC applications that follow this model, such as high performance visualisation software (e.g. ParaView or VisIT) or debugging software (Intel VTune, Cray Apprentice2, or NVIDIA NSight), have bespoke client-server implementations. We developed this library with the aim of having a re-useable framework that could be used to add interactive capabilities to a variety of HPC applications.

### Client-side 

The client-side library is located in `WSRTI/webappclient`

### Server-side

The server-side utilites include:

```
WSRTI/serializer
WSRTI/syncqueue
WSRTI/tjpp
WSRTI/websocketplus
```

### Attributions and other comments

This library was a collaborative effort, including work by:

Tim Dykes, HPE HPC/AI EMEA Research Lab
Ugo Varetto, Pawsey Supercomputing Center
Claudio Gheller, Institute of Radioastronomy, INAF
Mel Krokos, University of Portsmouth

A research publication documenting the library motivation, design, and use cases is currently under review, a link will be uploaded here when published.

WSRTI was used to enable the interactive version of the Splotch visualisation software (hosted [here](https://wwwmpa.mpa-garching.mpg.de/~kdolag/Splotch/) and [here](https://github.com/splotchviz/splotch)), a demonstrative use case is shown in [1]

[1]  T Dykes, A Hassan, C Gheller, D Croton, M Krokos, Interactive 3D visualization for theoretical virtual observatories, Monthly Notices of the Royal Astronomical Society, Volume 477, Issue 2, June 2018, Pages 1495–1507, https://doi.org/10.1093/mnras/sty855


